(ns defgen.core
  (:require [clojure.edn :as edn]
            [clojure.set :refer [difference]]
            [clojure.string :as s]
            [me.raynes.fs :as fs]
            [medley.core :refer [filter-vals map-vals]])
  (:gen-class))

;; THOUGHTS:
;; * We should put hidden rom-defs at the beginning though and then set
;;   their LFAs to zero.  That way the ROM dictionary search will
;;   terminate as soon the first hidden definition is found.
(defn id-to-token
  [id]
  (-> id
      name
      s/upper-case))

(defn id-to-name
  [id]
  (-> id
      name
      s/upper-case))

(defn escape-c-char
  [c]
  (condp = c
    \' "'\\''"
    \\ "'\\\\'"
    (str "'" c "'")))

(defn string-to-char-array
  [s]
  (map escape-c-char s))

(defn escape-c-string
  [orig]
  (-> orig
      (s/replace "\\" "\\\\")
      (s/replace "\"" "\\\"")))

(defn parse-def
  [{[args-in args-out] :args
    :keys [token name flags source pfa]
    :or {flags #{}}}]
  {:id token
   :token-name (id-to-token token)
   :name (or name (id-to-name token))
   :args-in args-in
   :args-out args-out
   :source source
   :pfa pfa
   :primitive-type (if (empty? pfa) :code :definition)
   :headerless? (flags :headerless)
   :immediate? (flags :immediate)})

(defn load-def-file
  "Returns a map of all of the definitions in the given file, indexed by token id."
  [path]
  (with-open [rdr (java.io.PushbackReader. (clojure.java.io/reader path))]
    (loop [defs []]
      (if-let [next-def (edn/read {:eof nil} rdr)]
        (recur (conj defs (parse-def next-def)))
        (into {} (map #(vector (% :id) %) defs))))))

(defn partition-defs
  "Returns [code-prims rom-defs] given a raw list of defs."
  [defs]
  [(filter-vals #(-> % :primitive-type (= :code)) defs)
   (filter-vals #(-> % :primitive-type (= :definition)) defs)])


;; =====================================================================
;; FUNCTIONS FOR WORKING WITH CODE PRIMITIVES
;;

(defn assign-token-values
  [code-prims]
  (into {} (map-indexed (fn [idx [k prim]]
                          (vector k (assoc prim :token-value idx)))
                        code-prims)))


;; =====================================================================
;; FUNCTIONS FOR WORKING WITH ROM DEFINITIONS
;;

;; FIXME: I don't love the sometimes-map, sometimes-just-call approach.
;; Yes, we use plurals to decide that, but it still seems weird.  I
;; think that I would either always map or never map (in the outer call
;; anyway).

(defn calc-header-size
  [{:keys [headerless? name] :as rom-def}]
  (let [name-size (if headerless? 0 (count name))
        header-size (+ 1 ;; PSF+5bit name length
                       2 ;; LFA
                       1)] ;; CFA
    (assoc rom-def ::name-size name-size ::header-size header-size)))

(defn token-byte-size
  [code-prims token]
  (cond
    (number? token)       1
    (string? token)       1
    (code-prims token)    1
    :else                 2))

(defn calc-token-list-size
  [code-prims token-list]
  (apply + (map #(token-byte-size code-prims %) token-list)))

(defn calc-pfa-size
  [code-prims {:keys [pfa] :as rom-def}]
  (let [pfa-size (calc-token-list-size code-prims pfa)]
    (assoc rom-def ::pfa-size pfa-size)))

(defn calc-xts
  [rom-defs]
  (let [name-sizes (map ::name-size rom-defs)
        prev-header-pfa-sizes (concat [0] (map #(+ (% ::header-size)
                                                   (% ::pfa-size))
                                               rom-defs))
        prev-header-pfa-this-name-size (map + prev-header-pfa-sizes name-sizes)
        xts (reductions + prev-header-pfa-this-name-size)]
    (map #(assoc %1 :xt (bit-or 0xC000 %2)) rom-defs xts)))

(defn xt-to-bytes
  "Convert the 16-bit XT into an MSB-first array of two bytes as C-style, hex-encoded strings."
  [offset]
  (format "0x%02X,0x%02X"
          (bit-and (bit-shift-right offset 8) 0xff)
          (bit-and offset 0xff)))

(defn build-headers
  [rom-defs]
  (let [prev-xts (concat [0] (map :xt rom-defs))]
    (map (fn [{:keys [headerless? immediate? name] :as rom-def} prev-xt]
           (assoc rom-def
                  ::header
                  (concat (when-not headerless?
                            (concat [(->> name reverse first escape-c-char (str "0x80|"))]
                                    (->> name reverse rest string-to-char-array)))
                          [(str (when immediate? "0x80|")
                                (if headerless? 0 (count name)))]
                          [(xt-to-bytes prev-xt)]
                          ["DOCOLONROM"])))
         rom-defs
         prev-xts)))

(defn extract-branch-span
  "Returns branch-span elements of the PFA vector starting at offset.  branch-span may be negative, in which case the span will start at offset minus branch-span and continue to, but not include, offset."
  [pfa offset branch-span]
  (let [start-offset (min offset (+ offset branch-span))
        end-offset (max offset (+ offset branch-span))]
    (subvec pfa start-offset end-offset)))

(defn adjust-branch-targets
  "Branch targets are based on the number of elements in the PFA, but when written out they need to be based on the number of bytes in the PFA (which can change if other ROM Definitions are being referenced, since those use two-byte XTs instead of one-byte tokens."
  [code-prims in-pfa]
  (loop [offset 0
         out-pfa []]
    (if (= offset (count in-pfa))
      out-pfa
      (let [token (nth in-pfa offset)]
        (cond
          (= :icharlit token)
          (let [offset (inc offset)
                constant (nth in-pfa offset)]
            (recur (inc offset)
                   (conj out-pfa
                         token
                         constant)))

          (#{:ibranch :izbranch :piqdo :piloop :piplusloop} token)
          (let [branch-target (nth in-pfa (inc offset))
                branch-span (extract-branch-span in-pfa
                                                 (inc offset)
                                                 branch-target)
                branch-byte-size (calc-token-list-size code-prims
                                                       branch-span)
                new-branch-target (if (pos? branch-target)
                                    branch-byte-size
                                    (* -1 branch-byte-size))]
            (recur (inc (inc offset))
                   (conj out-pfa
                         token
                         new-branch-target)))

          :else
          (recur (inc offset)
                 (conj out-pfa
                       token)))))))

(defn build-bodies
  [code-prims rom-defs]
  (let [rom-def-xts (->> rom-defs
                         (map #(vector (% :id) (% :xt)))
                         (into {}))]
    (map (fn [{:keys [pfa] :as rom-def}]
           (let [pfa (adjust-branch-targets code-prims pfa)
                 body (mapcat (fn [token]
                                (cond
                                  (number? token) [token]
                                  (string? token) [token]
                                  (code-prims token) [(:token-name (code-prims token))]
                                  (rom-def-xts token) [(xt-to-bytes (rom-def-xts token))]
                                  :else (throw (Exception. (str "Unknown token " token)))))
                              pfa)]
             (assoc rom-def ::body body)))
         rom-defs)))


;; =====================================================================
;; OUTPUT FUNCTIONS
;;

(defn print-names-table
  [code-prims]
  (doall (->> code-prims
              (map-vals (fn [{:keys [name token-name headerless?]}]
                          (println (if headerless?
                                     (format "\"\\000\" /* %s */"
                                             token-name)
                                     (format "\"\\%03o\" \"%s\""
                                             (count name)
                                             (escape-c-string name))))))))
  (println "\"\\377\""))

(defn print-token-enum
  [code-prims]
  (dorun
    (map-vals (fn [{:keys [token-name token-value]}]
                (println (format "%s = 0x%02x," token-name token-value)))
              code-prims)))

(defn print-jump-table
  [code-prims]
  (let [all-token-values (zipmap (range 0x70)
                                 (repeat nil))
        token-value-names (zipmap (map :token-value (vals code-prims))
                                  (map :token-name (vals code-prims)))
        token-list (sort-by key (merge
                                  all-token-values
                                  token-value-names))]
    (doseq [[token-value token-name] token-list]
      (println (if (nil? token-name)
                 "0,"
                 (str "&&" token-name ","))))))

(defn print-rom-defs-block
  [rom-defs]
  (doseq [{token-name :token-name
           headerless? :headerless?
           xt :xt
           header ::header
           name-size ::name-size
           body ::body} (sort-by :xt rom-defs)]
    (println "/*" token-name "*/")
    (when-not headerless?
      (print (s/join ", " (take name-size header)))
      (println ","))
    (println (format "#define ROMDEF_%s 0x%X" token-name xt))
    (print (s/join ", " (drop name-size header)))
    (println ",")
    (print (s/join ", " body))
    (println ",")
    (println))
  (println (format "#define ROMDEF_LAST 0x%X" (->> rom-defs last :xt))))

;; TODO: Use subgraph syntax "ROT -> { OVER DUP SWAP WHATEVER }
;; TODO: Eliminate EXIT, IZBRANCH, ZBRANCH, etc.
#_(defn print-rom-def-relationships
  [rom-defs]
  (doseq [{:keys [token-name pfa]} rom-defs]
    (doseq [token pfa]
      (when (keyword? token)
        (println token-name "->" (id-to-token token) ";")))))


;; =====================================================================
;; REPL HELPERS
;;

(comment
  (def cp (->> ["../primitives/core.edn"
                "../primitives/core-ext.edn"
                "../primitives/double.edn"
                "../primitives/enforth.edn"
                "../primitives/string.edn"
                "../primitives/tools.edn"]
               (map load-def-file)
               (apply merge)
               partition-defs
               first
               assign-token-values))
  (def rd (->> ["../primitives/core.edn"
                "../primitives/core-ext.edn"
                "../primitives/double.edn"
                "../primitives/enforth.edn"
                "../primitives/string.edn"
                "../primitives/tools.edn"]
               (map load-def-file)
               (apply merge)
               partition-defs
               second
               vals)))


;; =====================================================================
;; MAIN ENTRY POINT
;;

(defn -main
  [out-path & def-paths]
  (let [[code-prims rom-defs] (->> def-paths
                                   (map load-def-file)
                                   (apply merge)
                                   partition-defs)
        code-prims (->> code-prims
                        assign-token-values)
        rom-defs (->> rom-defs
                      vals
                      ;; FIXME We have to sort by id because POSTPONE
                      ;; depends on COMPILECOMMA and so we need
                      ;; COMPILECOMMA to come first.  It would be best
                      ;; if we didn't have to depend on having
                      ;; ROMDEF_PFA_COMPILECOMMA defined in order for
                      ;; POSTPONE to work.
                      (sort-by :id)
                      (map calc-header-size)
                      (map #(calc-pfa-size code-prims %))
                      calc-xts
                      build-headers
                      (build-bodies code-prims))]

    ;; Output statistics.
    (println "Number of code primitives :" (count code-prims))
    (println "Number of Forth primitives:" (count rom-defs))

    ;; Output the name list.
    (println "*** NAMES ***")
    (spit
      (fs/file out-path "enforth_names.h")
      (with-out-str
        (print-names-table code-prims)))

    ;; Output the token enums.
    (println "*** TOKEN ENUM ***")
    (spit
      (fs/file out-path "enforth_tokens.h")
      (with-out-str
        (print-token-enum code-prims)))

    ;; Output the jump table
    (println "*** JUMP TABLE ***")
    (spit
      (fs/file out-path "enforth_jumptable.h")
      (with-out-str
        (print-jump-table code-prims)))

    ;; Output the definition block.
    (println "*** DEFINITIONS ***")
    (spit
      (fs/file out-path "enforth_definitions.h")
      (with-out-str
        (print-rom-defs-block rom-defs)))))

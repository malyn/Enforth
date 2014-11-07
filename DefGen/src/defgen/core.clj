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
   :cfa (if pfa "DOCOLONROM" (id-to-token token))
   :source source
   :pfa pfa
   :code? (empty? pfa)
   :definition? (sequential? pfa)
   :hidden? (contains? flags :headerless)
   :immediate? (contains? flags :immediate)})

(defn def-compare
  "Compares defs by hidden/public, then name."
  [{this-hidden? :hidden? this-name :name}
   {that-hidden? :hidden? that-name :name}]
  (cond
    (not= this-hidden? that-hidden?) this-hidden?
    :else (compare this-name that-name)))

(defn sort-defs
  [defs]
  (into (sorted-map-by (fn [key1 key2]
                         (def-compare (defs key1) (defs key2))))
        defs))

(defn load-def-c-file
  [path]
  ;; Partition the lines into consecutive groups of EDN data
  ;; (***-prefixed lines) and non-EDN data.  The file will always
  ;; follow the pattern non-EDN, EDN, non-EDN, EDN, ... due to the
  ;; fact that the comment must begin and end on lines that do *not*
  ;; contain EDN data.  We use this to our advantage and strip out the
  ;; odd number elements in the sequence after the partition
  ;; operation.  Then we strip the comment leader from each line, join
  ;; all of the lines together, and finally run each joined line
  ;; through the EDN reader.
  (->> path
       slurp
       s/split-lines
       (partition-by #(some? (re-matches #"^\s*\*\*\*.*" %)))
       rest
       (take-nth 2)
       (map (fn [lines] (map #(s/replace % #"^\s*\*\*\*(.*)$" "$1")
                             lines)))
       (map s/join)
       (map edn/read-string)
       (map parse-def)))

(defn load-def-edn-file
  [path]
  (with-open [rdr (java.io.PushbackReader. (clojure.java.io/reader path))]
    (loop [defs []]
      (if-let [next-def (edn/read {:eof nil} rdr)]
        (recur (conj defs (parse-def next-def)))
        defs))))

(defn load-def-file
  "Returns a map of all of the definitions in the given file, indexed by token id.  The file may be either an EDN file (ending in .edn) or a C file (ending in .c).  If the latter, then ***-prefixed comments will be read in order to locate EDN data."
  [path]
  (let [defs (if (= ".c" (fs/extension path))
               (load-def-c-file path)
               (load-def-edn-file path))]
    (into {} (map #(vector (% :id) %) defs))))


;; =====================================================================
;; FUNCTIONS FOR WORKING WITH CODE PRIMITIVES
;;

(defn assign-token-values
  [defs]
  (->> defs
       (filter-vals :code?)
       sort-defs
       (reduce-kv (fn [indexed-defs k v]
                    (assoc indexed-defs k (assoc v
                                                 :token-value
                                                 (count indexed-defs))))
                  {})
       (merge defs)))


;; =====================================================================
;; FUNCTIONS FOR WORKING WITH ROM DEFINITIONS
;;

;; FIXME: I don't love the sometimes-map, sometimes-just-call approach.
;; Yes, we use plurals to decide that, but it still seems weird.  I
;; think that I would either always map or never map (in the outer call
;; anyway).

(defn calc-header-size
  [{:keys [hidden? name] :as rom-def}]
  (let [name-size (if hidden? 0 (count name))
        header-size (+ 1 ;; PSF+5bit name length
                       2 ;; LFA
                       1)] ;; CFA
    (assoc rom-def ::name-size name-size ::header-size header-size)))

(defn token-byte-size
  [defs token]
  (cond
    (number? token)            1
    (string? token)            1
    (some-> defs token :code?) 1
    :else                      2))

(defn calc-token-list-size
  [defs token-list]
  (apply + (map #(token-byte-size defs %) token-list)))

(defn calc-pfa-size
  [defs {:keys [pfa] :as rom-def}]
  (let [pfa-size (calc-token-list-size defs pfa)]
    (assoc rom-def ::pfa-size pfa-size)))

(defn assign-xts
  [rom-defs]
  (let [rom-defs (-> rom-defs sort-defs vals)
        name-sizes (map ::name-size rom-defs)
        prev-header-pfa-sizes (concat [0] (map #(+ (% ::header-size)
                                                   (% ::pfa-size))
                                               rom-defs))
        prev-header-pfa-this-name-size (map + prev-header-pfa-sizes name-sizes)
        xts (reductions + prev-header-pfa-this-name-size)]
    (zipmap (map :id rom-defs)
            (map #(assoc %1 :xt (bit-or 0xC000 %2)) rom-defs xts))))

(defn xt-to-bytes
  "Convert the 16-bit XT into an MSB-first array of two bytes as C-style, hex-encoded strings."
  [offset]
  (format "0x%02X,0x%02X"
          (bit-and (bit-shift-right offset 8) 0xff)
          (bit-and offset 0xff)))

(defn build-headers
  [rom-defs]
  (let [rom-def-vals (-> rom-defs sort-defs vals)
        prev-xts (concat [0] (map :xt rom-def-vals))
        rom-def-prev-xts (zipmap (map :id rom-def-vals)
                                 prev-xts)]
    (map-vals (fn [{:keys [id hidden? immediate? name cfa] :as rom-def}]
                (assoc rom-def
                       ::header
                       (concat (when-not hidden?
                                 (concat [(->> name reverse first escape-c-char (str "0x80|"))]
                                         (->> name reverse rest string-to-char-array)))
                               [(str (when immediate? "0x80|")
                                     (if hidden? 0 (count name)))]
                               [(xt-to-bytes (rom-def-prev-xts id))]
                               [cfa])))
              rom-defs)))

(defn extract-branch-span
  "Returns branch-span elements of the PFA vector starting at offset.  branch-span may be negative, in which case the span will start at offset minus branch-span and continue to, but not include, offset."
  [pfa offset branch-span]
  (let [start-offset (min offset (+ offset branch-span))
        end-offset (max offset (+ offset branch-span))]
    (subvec pfa start-offset end-offset)))

(defn adjust-branch-targets
  "branch targets are based on the number of elements in the pfa, but when written out they need to be based on the number of bytes in the pfa (which can change if other rom definitions are being referenced, since those use two-byte xts instead of one-byte tokens."
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
  [defs]
  (map-vals (fn [{:keys [pfa] :as rom-def}]
              (let [pfa (adjust-branch-targets defs pfa)
                    body (mapcat (fn [token]
                                   (cond
                                     (number? token) [token]
                                     (string? token) [token]
                                     (some-> defs token :code?) [(-> defs token :token-name)]
                                     (some-> defs token :definition?) [(xt-to-bytes (-> defs token :xt))]
                                     :else (throw (Exception. (str "Unknown token " token)))))
                                 pfa)]
                (assoc rom-def ::body body)))
            defs))


;; =====================================================================
;; OUTPUT FUNCTIONS
;;

(defn print-token-enum
  [defs]
  (let [code-prims (->> defs (filter-vals :code?) sort-defs vals)]
    (dorun
      (map (fn [{:keys [token-name token-value]}]
             (println (format "%s = 0x%02x," token-name token-value)))
           code-prims))))

(defn print-jump-table
  [defs]
  (let [code-prims (->> defs (filter-vals :code?) sort-defs vals)
        all-token-values (zipmap (range 0x70)
                                 (repeat nil))
        token-value-names (zipmap (map :token-value code-prims)
                                  (map :token-name code-prims))
        token-list (sort-by key (merge
                                  all-token-values
                                  token-value-names))]
    (doseq [[token-value token-name] token-list]
      (println (if (nil? token-name)
                 "0,"
                 (str "&&" token-name ","))))))

(defn print-rom-defs-block
  [defs]
  (println "#define ROMDEF_LAST 0x0000")
  (println)
  (doseq [{token-name :token-name
           hidden? :hidden?
           definition? :definition?
           xt :xt
           header ::header
           name-size ::name-size
           body ::body} (->> defs sort-defs vals)]
    (println "/*" token-name "*/")
    (when-not hidden?
      (print (s/join ", " (take name-size header)))
      (println ","))
    (println "#undef ROMDEF_LAST")
    (println (format "#define ROMDEF_%s 0x%X" token-name xt))
    (println (format "#define ROMDEF_LAST 0x%X" xt))
    (print (s/join ", " (drop name-size header)))
    (println ",")
    (when definition?
      (print (s/join ", " body))
      (println ","))
    (println)))

;; TODO: Use subgraph syntax "ROT -> { OVER DUP SWAP WHATEVER }
;; TODO: Eliminate EXIT, IZBRANCH, ZBRANCH, etc.
#_(defn print-rom-def-relationships
  [rom-defs]
  (doseq [{:keys [token-name pfa]} rom-defs]
    (doseq [token pfa]
      (when (keyword? token)
        (println token-name "->" (id-to-token token) ";")))))


;; =====================================================================
;; COMPILATION FUNCTION
;;

(defn compile-definitions
  [defs]
  (->> defs
       assign-token-values
       (map-vals calc-header-size)
       (map-vals #(calc-pfa-size defs %))
       assign-xts
       build-headers
       build-bodies))


;; =====================================================================
;; REPL HELPERS
;;

(comment
  (def ds (->> ["../primitives/core.edn"
                "../primitives/core-ext.edn"
                "../primitives/double.edn"
                "../primitives/enforth.edn"
                "../primitives/string.edn"
                "../primitives/tools.edn"]
               (map load-def-file)
               (apply merge)
               compile-definitions)))


;; =====================================================================
;; MAIN ENTRY POINT
;;

(defn -main
  [out-path & def-paths]
  (let [defs (->> def-paths
                  (map load-def-file)
                  (apply merge)
                  compile-definitions)]

    ;; Output statistics.
    (println "Number of code primitives :" (->> defs (filter-vals :code?) count))
    (println "Number of Forth primitives:" (->> defs (filter-vals :definition?) count))

    ;; Output the token enums.
    (println "*** TOKEN ENUM ***")
    (spit
      (fs/file out-path "enforth_tokens.h")
      (with-out-str
        (print-token-enum defs)))

    ;; Output the jump table
    (println "*** JUMP TABLE ***")
    (spit
      (fs/file out-path "enforth_jumptable.h")
      (with-out-str
        (print-jump-table defs)))

    ;; Output the definition block.
    (println "*** DEFINITIONS ***")
    (spit
      (fs/file out-path "enforth_definitions.h")
      (with-out-str
        (print-rom-defs-block defs)))))

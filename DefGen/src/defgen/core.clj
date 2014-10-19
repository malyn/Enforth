(ns defgen.core
  (:require [clojure.edn :as edn]
            [clojure.set :refer [difference]]
            [clojure.string :as s]
            [me.raynes.fs :as fs])
  (:gen-class))

;; INPUT PHASE:
;; 1. Load all defs.
;; 2. Sort defs.
;; 3. Split defs into code-prims and rom-defs.
;;
;; CODE-PRIMS PROCESSING:
;; 1. Assign tokens to code-prims.
;;
;; ROM-DEFS PROCESSING:
;; 1. Calculate header size (2 + 1 + name + 1)
;; 2. Calculate size of each PFA.
;; 3. Calculate offset of each definition.
;; 4. Build header.
;;
;; CODE-PRIMS OUTPUT PHASE:
;; 1. Write out code-prims names table.
;; 2. Write out code-prims jumptable.
;; 3. Write out code-prims tokens table.
;;
;; ROM-DEFS OUTPUT PHASE:
;; 1. Write out rom-defs definitions block.
;;
;; THOUGHTS:
;; * Could avoid sorting at all and just use the file order.  That will
;;   be just as stable as sorting, but not require that we (constantly
;;   re-)sort in DefGen.  Also, we could just generate a map during
;;   loading and then use that everywhere rather than passing seqs
;;   around.  Assign tokens could be map'ing with the seq and an
;;   infinite range sequence, for example.  Probably a TODO for after
;;   this current change works (as a simplification/refactoring change).
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
  "Returns a vector of all of the definitions in the given file."
  [path]
  (with-open [rdr (java.io.PushbackReader. (clojure.java.io/reader path))]
    (loop [defs []]
      (if-let [next-def (edn/read {:eof nil} rdr)]
        (recur (conj defs (parse-def next-def)))
        defs))))

(defn compare-def
  [this that]
  (let [[this-private? that-private?] (map #(-> % :headerless? boolean)
                                           [this that])
        [this-name that-name] (map :name [this that])]
    (cond
      (not= this-private? that-private?) that-private?
      :else (compare this-name that-name))))

(defn sort-defs
  [defs]
  (sort-by identity compare-def defs))

(defn partition-defs
  "Returns [code-prims rom-defs] given a raw list of defs."
  [defs]
  [(filter #(-> % :primitive-type (= :code)) defs)
   (filter #(-> % :primitive-type (= :definition)) defs)])


;; =====================================================================
;; FUNCTIONS FOR WORKING WITH CODE PRIMITIVES
;;

(defn assign-token-values
  [code-prims]
  (map-indexed (fn [idx prim] (assoc prim :token-value idx)) code-prims))


;; =====================================================================
;; FUNCTIONS FOR WORKING WITH ROM DEFINITIONS
;;

;; FIXME: I don't love the sometimes-map, sometimes-just-call approach.
;; Yes, we use plurals to decide that, but it still seems weird.  I
;; think that I would either always map or never map (in the outer call
;; anyway).

(defn calc-header-size
  [{:keys [headerless? name] :as rom-def}]
  (let [header-size (+ 2 ;; LFA
                       1 ;; PSF+5bit name length
                       (if headerless? 0 (count name)) ;; NFA
                       1)] ;; CFA
    (assoc rom-def ::header-size header-size)))

(defn token-byte-size
  [code-prim-ids token]
  (cond
    (number? token)       1
    (string? token)       1
    (code-prim-ids token) 1
    :else                 2))

(defn calc-token-list-size
  [code-prim-ids token-list]
  (apply + (map #(token-byte-size code-prim-ids %) token-list)))

(defn calc-pfa-size
  [code-prims {:keys [pfa] :as rom-def}]
  (let [code-prim-ids (->> code-prims
                           (map #(vector (:id %) %))
                           (into {}))
        pfa-size (calc-token-list-size code-prim-ids pfa)]
    (assoc rom-def ::pfa-size pfa-size)))

(defn calc-offsets
  [rom-defs]
  (let [offsets (reductions + 0 (map #(+ (% ::header-size) (% ::pfa-size))
                                     rom-defs))]
    (map #(assoc %1 :offset %2)
         rom-defs
         offsets)))

(defn lfa-to-bytes
  "Convert the 16-bit LFA into an LSB-first array of two bytes."
  [offset]
  [(bit-and offset 0xff)
   (bit-and (bit-shift-right offset 8) 0xff)])

(defn build-headers
  [rom-defs]
  (let [lfa-vals (concat [0] (map #(+ 0xC000 (% :offset)) rom-defs))]
    (map (fn [{:keys [headerless? immediate? name] :as rom-def} lfa]
           (assoc rom-def
                  ::header
                  (concat (lfa-to-bytes lfa)
                          [(+ (if immediate? 0x80 0)
                              (if headerless? 0 (count name)))]
                          (when-not headerless? (string-to-char-array name))
                          ["DOCOLONROM"])))
         rom-defs
         lfa-vals)))

(defn xt-to-bytes
  "Convert the 16-bit XT into an MSB-first array of two bytes."
  [offset]
  [(bit-and (bit-shift-right offset 8) 0xff)
   (bit-and offset 0xff)])

(defn extract-branch-span
  "Returns branch-span elements of the PFA vector starting at offset.  branch-span may be negative, in which case the span will start at offset minus branch-span and continue to, but not include, offset."
  [pfa offset branch-span]
  (let [start-offset (min offset (+ offset branch-span))
        end-offset (max offset (+ offset branch-span))]
    (subvec pfa start-offset end-offset)))

(defn adjust-branch-targets
  "Branch targets are based on the number of elements in the PFA, but when written out they need to be based on the number of bytes in the PFA (which can change if other ROM Definitions are being referenced, since those use two-byte XTs instead of one-byte tokens."
  [code-prim-ids in-pfa]
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
                branch-byte-size (calc-token-list-size code-prim-ids
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
  (let [code-prim-ids (->> code-prims
                           (map #(vector (% :id) %))
                           (into {}))
        rom-def-cfa-offsets (->> rom-defs
                                 (map #(vector (% :id)
                                               (+ (% :offset)
                                                  (% ::header-size)
                                                  -1))) ;; Back up to CFA
                                 (into {}))]
    (map (fn [{:keys [pfa] :as rom-def}]
           (let [pfa (adjust-branch-targets code-prim-ids pfa)
                 body (mapcat (fn [token]
                                (cond
                                  (number? token) [token]
                                  (string? token) [token]
                                  (code-prim-ids token) [(:token-name (code-prim-ids token))]
                                  (rom-def-cfa-offsets token) (xt-to-bytes (bit-or 0xC000 (rom-def-cfa-offsets token)))
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
              (filter #(not (% :headerless?)))
              (map (fn [{:keys [name immediate?]}]
                     (println (format "\"\\%03o\" \"%s\""
                                      (bit-or (if immediate? 0x80 0x00)
                                              (count name))
                                      (escape-c-string name)))))))
  (println "\"\\377\""))

(defn print-token-enum
  [code-prims]
  (dorun
    (map (fn [{:keys [token-name token-value]}]
           (println (format "%s = 0x%02x," token-name token-value)))
         code-prims)))

(defn print-jump-table
  [code-prims]
  (let [all-token-values (zipmap (range 0x70)
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
  [rom-defs]
  (doseq [{token-name :token-name
           offset :offset
           header ::header
           header-size ::header-size
           body ::body} rom-defs]
    (println "/*" token-name "*/")
    (print (s/join ", " header))
    (println ",")
    (println (str "#define ROMDEF_PFA_" token-name) (+ offset header-size))
    (print (s/join ", " body))
    (println ",")
    (println))
  (println (str "#define ROMDEF_LAST " (->> rom-defs last :offset))))

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
               (mapcat load-def-file)
               sort-defs
               partition-defs
               first
               assign-token-values))
  (def cpids (->> cp
                  (map #(vector (% :id) %))
                  (into {}))))


;; =====================================================================
;; MAIN ENTRY POINT
;;

(defn -main
  [out-path & def-paths]
  (let [[code-prims rom-defs] (->> def-paths
                                   (mapcat load-def-file)
                                   sort-defs
                                   partition-defs)
        code-prims (->> code-prims
                        assign-token-values)
        rom-defs (->> rom-defs
                      (map calc-header-size)
                      (map #(calc-pfa-size code-prims %))
                      calc-offsets
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

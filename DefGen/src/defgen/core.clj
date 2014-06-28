(ns defgen.core
  (:require [clojure.edn :as edn]
            [clojure.set :refer [difference]]
            [clojure.string :as s]
            [me.raynes.fs :as fs])
  (:gen-class))

(def token-multiplier 8)

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
   :pfa (when pfa (concat pfa (let [pad (mod (count pfa) token-multiplier)]
                                (when (> pad 0)
                                  (repeat (- token-multiplier pad) 0)))))
   :primitive-type (if (empty? pfa) :code :definition)
   :headerless? (flags :headerless)
   :immediate? (flags :immediate)})

(defn load-def
  "Returns a vector of all of the definitions in the given file."
  [path]
  (with-open [rdr (java.io.PushbackReader. (clojure.java.io/reader path))]
    (loop [defs []]
      (if-let [next-def (edn/read {:eof nil} rdr)]
        (recur (conj defs (parse-def next-def)))
        defs))))

(defn compare-primitives
  [this that]
  (let [[this-def? that-def?] (map #(-> % :primitive-type (= :definition))
                                   [this that])
        [this-private? that-private?] (map #(-> % :headerless? boolean)
                                           [this that])
        [this-name that-name] (map :name [this that])]
    (cond
      (not= this-def? that-def?) this-def?
      (not= this-private? that-private?) that-private?
      :else (compare this-name that-name))))

(defn ordered-primitives
  "Orders the given primitives for minimal ROM usage:

  1. Sort all of the primitives.
  2. Assign token ids to all of the Forth-based primitives; first the public definitions, then the private definitions.
  3. Fill in the gaps in the token id list with code primitives; again, first the public primitives and then the private primitives."
  [prims]
  (let [prims (sort-by identity compare-primitives prims)
        prim-defs (filter #(-> % :primitive-type (= :definition)) prims)
        prim-defs-token-values (reductions + 0 (map #(-> % :pfa count (/ token-multiplier))
                                                    prim-defs))
        prim-defs (map #(assoc %1 :token-value %2)
                       prim-defs
                       prim-defs-token-values)
        code-defs (map #(assoc %1 :token-value %2)
                       (filter #(-> % :primitive-type (= :code)) prims)
                       (difference (apply sorted-set (range 0xE0))
                                   (set prim-defs-token-values)))]
    (sort-by :token-value (concat prim-defs code-defs))))

(defn pfa-elem-to-source
  [elem]
  (if (keyword? elem)
    (id-to-token elem)
    elem))

(defn -main
  [out-path & def-paths]
  (let [defs (->> def-paths
                  (mapcat load-def)
                  ordered-primitives)]
    ;; Output the name list.
    (println "*** NAMES ***")
    (spit
      (fs/file out-path "enforth_names.h")
      (with-out-str
        (dorun
          (let [def-names
                (into {}
                      (map (fn [{:keys [name token-name token-value headerless? immediate?]}]
                             [token-value
                              (if headerless?
                                (format "\"\\%03o\" /* %s */",
                                        (if immediate? 0x03 0x02)
                                        token-name)
                                (format "\"\\%03o\" \"%s\""
                                        (bit-or (if immediate? 0x03 0x02)
                                                (bit-shift-left (count name) 3))
                                        (escape-c-string name)))])
                           defs))
                possible-empty-names (into {} (map #(vector % "\"\\000\"")
                                                   (range (apply max
                                                                 (map :token-value defs)))))
                all-def-names (map val (sort-by key (merge possible-empty-names def-names)))]
            (doseq [name all-def-names]
              (println name))
            (println "\"\\377\"")))))

    ;; Output the token enums.
    (println "*** TOKEN ENUM ***")
    (spit
      (fs/file out-path "enforth_tokens.h")
      (with-out-str
        (dorun
          (map (fn [{:keys [token-name token-value]}]
                 (println (format "%s = 0x%02x," token-name token-value)))
               defs))))

    ;; Output the jump table
    (println "*** JUMP TABLE ***")
    (spit
      (fs/file out-path "enforth_jumptable.h")
      (with-out-str
        (dorun
          (let [code-tokens
                (into {}
                      (->> defs
                           (filter #(-> % :primitive-type (= :code)))
                           (map (fn [{:keys [token-name token-value]}]
                                  [token-value token-name]))))
                def-tokens
                (into {}
                      (->> defs
                           (filter #(-> % :primitive-type (= :definition)))
                           (map (fn [{:keys [token-value]}]
                                  [token-value "DOCOLONROM"]))))
                possible-unused-tokens (into {} (map #(vector % nil) (range 0xE0)))
                all-tokens (sort-by key (merge
                                          possible-unused-tokens
                                          def-tokens
                                          code-tokens))]
            (doseq [[token-value token-name] all-tokens]
              (println (if (nil? token-name)
                         "0,"
                         (str "&&" token-name ","))))))))

    ;; Output the definition block.
    (println "*** DEFINITIONS ***")
    (spit
      (fs/file out-path "enforth_definitions.h")
      (with-out-str
        (doseq [{:keys [token-name pfa]}
                (filter #(-> % :primitive-type (= :definition)) defs)]
          (println "/*" token-name "*/")
          (print (s/join ", " (map pfa-elem-to-source pfa)))
          (println ",")
          (println))))))

(ns defgen.core-test
  (:require [clojure.test :refer :all]
            [defgen.core :refer :all]))

(def code-prim-ids
  {:dup {}
   :drop {}
   :exit {}
   :ibranch {}
   :izbranch {}})

(deftest abt-no-branches
  (is (= [:dup :drop :exit]
         (adjust-branch-targets
           code-prim-ids
           [:dup :drop :exit]))))

(deftest abt-forward-branch
  (is (= [:ibranch 2 :dup :drop :exit]
         (adjust-branch-targets
           code-prim-ids
           [:ibranch 2 :dup :drop :exit]))))

(deftest abt-backward-branch
  (is (= [:dup :drop :ibranch -2 :exit]
         (adjust-branch-targets
           code-prim-ids
           [:dup :drop :ibranch -2 :exit]))))

(deftest abt-backward-branch-over-romdef
  (is (= [:dup :romdef :ibranch -3 :exit]
         (adjust-branch-targets
           code-prim-ids
           [:dup :romdef :ibranch -2 :exit]))))

(deftest abt-backward-branch-to-start
  (is (= [:dup :ibranch -2 :exit]
         (adjust-branch-targets
           code-prim-ids
           [:dup :ibranch -2 :exit]))))

(deftest abt-backward-branch-over-romdef-to-start
  (is (= [:romdef :ibranch -3 :exit]
         (adjust-branch-targets
           code-prim-ids
           [:romdef :ibranch -2 :exit]))))

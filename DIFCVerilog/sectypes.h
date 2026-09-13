#ifndef SECTYPES_H
#define SECTYPES_H
/*
 * Copyright (c) 1998-2013 Danfeng Zhang (zhangdf@cs.cornell.edu)
 * DIFCVerilog modifications Copyright (c) 2026 Yubo Shi
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

# include  <set>
# include  <string>
# include  "StringHeap.h"
#include <cstring>
#include <regex>
#include <fstream>
#include <iomanip>
# include  "PGate.h"
#include  "Statement.h"
#include <malloc.h>

class SecType;
class ConstType;
class VarType;
class JoinType;
class IndexType;
struct TypeEnv;
class Statement;
class PExpr;
class PAssign;
class PAssignNB;
class SecType {

    public:
        int secvalue;
        int ref_count = 0;
        SecType(int value) : secvalue(value) {}
        std::set<int> secvalues;
        std::set<int> basic;
        std::set<int> positive;
        std::set<int> negative;

        void release() {
            ref_count--;
            if (ref_count <= 0) {
                delete this;
            }
        }

        SecType() : secvalue(0) {}

        SecType(const std::set<int>& values) : secvalues(values) {}

        SecType(const std::set<int>& b, const std::set<int>& p, const std::set<int>& n)
            : basic(b), positive(p), negative(n) {}

        virtual ~SecType() {}
        virtual void dump(std::ostream&o) {}
        virtual void setTypeName(const perm_string& newName) {}
        virtual void changevalue(int delta) {}
        virtual bool hasBottom() {return false;}
        virtual bool isBottom() {return false;}
        virtual bool hasTop() {return false;}
        virtual bool isTop() {return false;};
        virtual int getSecvalue() {return secvalue;}
        virtual SecType* simplify() {return this;}
        virtual bool equals(SecType* st) {return false;}
        virtual SecType* subst(perm_string e1, perm_string e2) {return this;};
        virtual SecType* subst(map<perm_string, perm_string> m) {return this;};
        virtual void collect_dep_expr(set<perm_string>& m) {};
        virtual bool hasExpr(perm_string str) {return false;};
        virtual SecType* freshVars(unsigned int lineno, map<perm_string, perm_string>& m) {return this;};

        static bool compareSets(const SecType* st1, const SecType* st2) {
            if (!st1 || !st2) return false;

            return  ((st1->basic.empty() && st2->basic.empty()) ||
                    ((st1->basic.size() == 1 && *st1->basic.begin() == 1) &&
                     (st2->basic.size() == 1 && *st2->basic.begin() == 1))) &&
                    st1->positive.empty() &&
                    st1->negative.empty() &&
                    st2->positive.empty() &&
                    st2->negative.empty();
        }
};


// Kept for old parser/pform interfaces; current DIFCVerilog does not use it.
struct SecMaxType {
    SecMaxType() {}
    SecMaxType(int) {}
    SecMaxType(perm_string) {}
    explicit SecMaxType(const std::set<int>&) {}
    SecMaxType(const SecMaxType&) {}
    static SecMaxType* TOP;
    static SecMaxType* BOT;
};

// Kept for old parser/pform interfaces; current DIFCVerilog does not use it.
struct SecDownType {
    SecDownType() {}
    SecDownType(int) {}
    SecDownType(perm_string) {}
    explicit SecDownType(const std::set<int>&) {}
    SecDownType(const SecDownType&) {}
    static SecDownType* TOP;
    static SecDownType* BOT;
};

class ConstType : public SecType {
public:
    int secvalue;

    ConstType():secvalue(0) {  }
    ConstType(int value) : secvalue(value) {}
    ConstType(perm_string name);
    ~ConstType();
    void dump(ostream& o) {
        o << name;
    }
    bool hasBottom() {
        return name == "LOW";
    }
    bool hasTop() {
        return name == "HIGH";
    }
    bool isBottom() {
        return name == "LOW";
    }
    bool isTop() {
        return name == "HIGH";
    }
    bool equals(SecType* st);
    SecType* freshVars(unsigned int lineno, map<perm_string, perm_string>& m);

public:
    static ConstType* TOP;
    static ConstType* BOT;

private:
    perm_string name;
};

class VarType : public SecType {
    public:
      VarType(perm_string varname);
      ~VarType();
      VarType& operator= (const VarType&);

    public:
      void set_type(perm_string varname);
      perm_string get_type() const;
      bool equals(SecType* st);
      SecType* freshVars(unsigned int lineno, map<perm_string, perm_string>& m);
      bool hasExpr(perm_string str);

    private:
      perm_string varname_;
};

class IndexType : public SecType {

    public:
	  IndexType(perm_string name, perm_string expr);
      ~IndexType();
      IndexType& operator= (const IndexType&);
      void dump(ostream&o) {
      	o << "(" << name_ << " " << expr_ << ")";
      }
      bool equals(SecType* st);

    public:
      void set_type(const perm_string name , perm_string expr);
      perm_string get_name() const;
      perm_string get_expr() const;
      SecType* subst(perm_string e1, perm_string e2);
      SecType* subst(map<perm_string, perm_string> m);
      void collect_dep_expr(set<perm_string>& m);
      SecType* freshVars(unsigned int lineno, map<perm_string, perm_string>& m);
      bool hasExpr(perm_string str);

    public:
      static IndexType* RL;
      static IndexType* WL;

    private:
      perm_string name_;
      perm_string expr_;
};

class JoinType : public SecType {

    public:
	  JoinType(SecType*, SecType*);
      ~JoinType();
      JoinType& operator= (const JoinType&);
      void dump(ostream&o) {
          o << "(join ";
          comp1_->dump(o);
          o << " ";
          comp2_->dump(o);
          o << ")";
      }
      SecType* getFirst();
      SecType* getSecond();

      bool hasBottom() {return comp1_->hasBottom() || comp2_->hasBottom();}
      bool isBottom() {return comp1_->isBottom() && comp2_->isBottom();}
      bool hasTop() {return comp1_->hasTop() || comp2_->hasTop();}
      bool isTop() {return comp1_->isTop() || comp2_->isTop();}
      SecType* simplify();
      int getSecvalue();
      bool equals(SecType* st);
      int compare(SecType* st, SecType* nt);
      SecType* subst(perm_string e1, perm_string e2);
      SecType* subst(map<perm_string, perm_string> m);
      void collect_dep_expr(set<perm_string>& m);
      SecType* freshVars(unsigned int lineno, map<perm_string, perm_string>& m);
      bool hasExpr(perm_string str);

	  SecType* comp1_;
	  SecType* comp2_;
};

class MeetType : public SecType {

    public:
	  MeetType(SecType*, SecType*);
      ~MeetType();
      MeetType& operator= (const MeetType&);
      void dump(ostream&o) {
          o << "(meet ";
          comp1_->dump(o);
          o << " ";
          comp2_->dump(o);
          o << ")";
      }
      SecType* getFirst();
      SecType* getSecond();

      bool hasBottom() {return comp1_->hasBottom() || comp2_->hasBottom();}
      bool isBottom() {return comp1_->isBottom() || comp2_->isBottom();}
      bool hasTop() {return comp1_->hasTop() || comp2_->hasTop();}
      bool isTop() {return comp1_->isTop() && comp2_->isTop();}
      SecType* simplify();
      bool equals(SecType* st);
      SecType* subst(perm_string e1, perm_string e2);
      SecType* subst(map<perm_string, perm_string> m);
      void collect_dep_expr(set<perm_string>& m);
      SecType* freshVars(unsigned int lineno, map<perm_string, perm_string>& m);
      bool hasExpr(perm_string str);

    private:
	  SecType* comp1_;
	  SecType* comp2_;
};

struct Hypothesis {
    PExpr* bexpr_;

    Hypothesis(const Hypothesis& other)
    : bexpr_(other.bexpr_ ? other.bexpr_->clone() : nullptr)
{

}

    Hypothesis(PExpr* l, PExpr* r) {
        if (l && r) {
            PExpr* new_l = l->clone();
            PExpr* new_r = r->clone();
            bexpr_ = new PEBComp('e', new_l, new_r);
        } else {
            bexpr_ = nullptr;
        }
    }

    Hypothesis(PExpr* bexpr) {
        bexpr_ = bexpr ? bexpr->clone() : nullptr;
    }

    Hypothesis& operator=(const Hypothesis& other) {
        if (this != &other) {
            delete bexpr_;
            bexpr_ = other.bexpr_ ? other.bexpr_->clone() : nullptr;
        }
        return *this;
    }

    ~Hypothesis() {
        delete bexpr_;
    }

    PExpr* getL() const {
        if (!bexpr_) return nullptr;
        if (bexpr_ && dynamic_cast<PEBComp*>(bexpr_)) {
            return static_cast<PEBComp*>(bexpr_)->left_;
        }
        return nullptr;
    }

    Hypothesis* subst(map<perm_string, perm_string> m);
    bool matches(perm_string name);
};

class HypothesisComparator
{
public:
    bool operator()(const Hypothesis* h1, const Hypothesis* h2) const
    {
         if (!h1 || !h2) return !h1 && h2;
        if (!h1->bexpr_ || !h2->bexpr_)
            return !h1->bexpr_ && h2->bexpr_;
        return h1->bexpr_->get_name() < h2->bexpr_->get_name();
    }
};

struct Predicate {
	set<Hypothesis*> hypotheses;

    Predicate& operator= (const Predicate&);
	Predicate* subst (map<perm_string, perm_string> m);
};

struct Equality {
	SecType* left;
	SecType* right;
	bool isleq;

	Equality(SecType* l, SecType* r, bool leq = false) {
		left = l;
		right = r;
		isleq = leq;
	}

	void dump(ostream&out) const;
	Equality* subst(map<perm_string, perm_string> m);
};

struct Invariant {
    set<Equality*> invariants;
};

static inline std::string escape_quote(const std::string& s)
{
    if (s.empty()) {
        return "";
    }
    std::string r;
    r.reserve(s.size() + 16);
    for (char c : s) {
        if (c == '"')  r += "\\\"";
        else           r += c;
    }
    return r;
}
 struct UnresolvedDependency  {
    const Statement* statement;
    const PExpr* expression;
    int line_number;
     Predicate pred;
    string file;
};
struct TypeEnv {
    map<perm_string, SecType*>* varsToType;

    SecType* pc;
    // Legacy fields kept for parser/pform compatibility; DIFCVerilog does not read them.
    SecMaxType* pc_m;
    SecDownType* pc_d;
    Module* module;
    Invariant* invariants;
    perm_string current_instance_name;
    perm_string current_parent_name;
    string pcstring;

    set<perm_string> dep_exprs;
    set<perm_string> aliveVars;

    TypeEnv(map<perm_string, SecType*>* m,
            SecType* pclabel, SecMaxType* pc_max, SecDownType* pc_down, Module* modu)
    {
        varsToType = m;

        pc = pclabel;
        pc_m = pc_max;
        pc_d = pc_down;
        module = modu;
        invariants = new Invariant();
    }

    TypeEnv(map<perm_string, SecType*>& m,
            SecType* pclabel, SecMaxType* pc_max, SecDownType* pc_down, Module* modu)
    {
        varsToType = &m;

        pc = pclabel;
        pc_m = pc_max;
        pc_d = pc_down;
        module = modu;
        invariants = new Invariant();
    }

    void addInvariant(Equality* inv) {
        invariants->invariants.insert(inv);
    }

    TypeEnv& operator= (const TypeEnv&);

    void release_memory() {
        if (varsToType) {
            for (auto& pair : *varsToType) {
                delete pair.second;
            }
            map<perm_string, SecType*>().swap(*varsToType);
        }

        if (invariants) {
            for (Equality* eq : invariants->invariants) {
                delete eq;
            }
            set<Equality*>().swap(invariants->invariants);
            delete invariants;
            invariants = nullptr;
        }

        set<perm_string>().swap(dep_exprs);

        malloc_trim(0);
    }


    void prepare_memory() {
        if (!varsToType) varsToType = new map<perm_string, SecType*>();
    }

};

class EnvRestorer {
    TypeEnv& env;
    std::function<void(TypeEnv&)> rebuilder;

public:
    EnvRestorer(TypeEnv& e, std::function<void(TypeEnv&)> f)
        : env(e), rebuilder(f) {
        env.release_memory();
    }

    ~EnvRestorer() {
        env.prepare_memory();
        rebuilder(env);
    }
};

struct Constraint {
	Predicate* pred;
	Invariant* invariant;
    std::string note;

    std::string ls;
    std::string rs;
    set<std::string> basicVars;
    std::vector<std::string> rs_tokens;
    std::vector<std::string> ls_tokens;
    map<perm_string, SecType*>* varsToType;
    int direct;
    std::string pc_positive;
    std::string pc_negative;
    std::string mod_name;
    std::string mor_name;
	Constraint( Invariant* inv, Predicate* p,  std::string no,  std::string ls1, std::string rs1, set<std::string> brs, std::vector<std::string> rstok, std::vector<std::string> lstok,map<perm_string, SecType*>*vars,int dir, std::string pc_p,std::string pc_n, std::string mod, std::string mor){
		pred = p;
		invariant = inv;
        note = no.empty() ? "unknown_constraint" : no;
        ls = ls1.empty() ? "unknown_ls" : ls1;
        rs = rs1.empty() ? "unknown_rs" : rs1;
        basicVars = brs;
        rs_tokens = rstok;
        ls_tokens = lstok;
        varsToType = vars;
        direct = dir;

        pc_positive = pc_p.empty() ? "1" : pc_p;
        pc_negative = pc_n.empty() ? "0" : pc_n;
        mod_name = mod.empty() ? "unknown_mod" : mod;
        mor_name = mor.empty() ? "unknown_mor" : mor;


	}
};

inline ostream& operator << (ostream&o, SecType&t)
{
	t.dump(o);
    return o;
}

inline ostream& operator << (ostream&o, Predicate& pred)
{
	set<Hypothesis*> l = pred.hypotheses;
	set<Hypothesis*>::iterator i = l.begin();
	if (i != l.end()) {
		o << "(";
		(*i)->bexpr_->dump(o);
		o << ")";
		i++;
	}
	for (; i != l.end() ; i++) {
		o << " ("; (*i)->bexpr_->dump(o); o << ")";
	}
	return o;
}

inline ostream& operator << (ostream&o, Invariant& invs)
{
	set<Equality*> l = invs.invariants;
	set<Equality*>::iterator i = l.begin();
	if (i != l.end()) {
		o << "(";
		(*i)->dump(o);
		o << ")";
		i++;
	}
	for (; i != l.end() ; i++) {
		o << " ("; (*i)->dump(o); o << ")";
	}
	return o;
}


inline  void printSet(const std::set<int>& s, std::ostream& out) {
    auto it = s.begin();
    if (it != s.end()) {
        out << *it;
        ++it;
    }
    while (it != s.end()) {
        out << "," << *it;
        ++it;
    }
}

inline ostream& operator << (ostream&o, Constraint&c)
{
    std::string cnote;
    try {
        cnote = escape_quote(c.note);
        if (cnote.empty()) {
            cnote = "unknown_constraint";
        }
    } catch (...) {
        cnote = "error_processing_note";
    }

    std::vector<std::string> rs_tokens_start;
    try {
        rs_tokens_start = c.rs_tokens;
    } catch (...) {
        rs_tokens_start = {"N_L"};
    }
    std::vector<std::string> rs_tokens=rs_tokens_start;
    std::vector<std::string> ls_tokens_start;
    try {
        ls_tokens_start =c.ls_tokens;
    } catch (...) {
        ls_tokens_start = {"N_L"};
    }

    std::string pc_p_output = "\"pc_p=" + c.pc_positive + "\" ";
    std::string mor_rs_output =  c.mor_name + "_" + c.rs + " ";
    std::string mor_ls_output =  c.mor_name + "_" + c.ls + " ";
    std::string note_output = "\"" + c.note + "\" ";

    std::vector<std::string> ls_tokens=ls_tokens_start;
    int count = 1;
    if (c.direct == 0)
    {
        for (auto& l : ls_tokens) {
            for (auto& r : rs_tokens) {
                std::string mod_r_output =   c.mod_name + "_" + r + " ";
                std::string mod_l_output =   c.mod_name + "_" + l + " ";

                o << note_output<<mod_r_output<<mod_l_output<<pc_p_output<<endl;
                count++;
            }
        }
    }
    else {
        if (c.direct == 1) {
            for (auto& r : rs_tokens) {
                o << note_output<<c.mod_name <<"_" << r <<" "<<mor_ls_output<<pc_p_output<< endl;
            }
        }
        else if (c.direct == 2) {
            o << note_output<<mor_rs_output<<c.mod_name <<"_" << ls_tokens[0] << " "<<pc_p_output<< endl;
        }
    }

    return o;
}

#endif

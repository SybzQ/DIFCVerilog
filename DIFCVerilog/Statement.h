#ifndef __Statement_H
#define __Statement_H
/*
 * Copyright (c) 1998-2008 Stephen Williams (steve@icarus.com)
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

# include  <string>
# include  "ivl_target.h"
# include  "svector.h"
# include  "StringHeap.h"
# include  "PDelays.h"
# include  "PExpr.h"
# include  "HName.h"
# include  "LineInfo.h"
# include  "sectypes.h"
class PExpr;
class Statement;
class PEventStatement;
class Design;
class NetAssign_;
class NetCAssign;
class NetDeassign;
class NetForce;
class NetScope;
class Hypothesis;
struct Predicate;
class PScope;
class NetProc;

/*
 * The PProcess is the root of a behavioral process. Each process gets
 * one of these, which contains its type (initial or always) and a
 * pointer to the single statement that is the process. A module may
 * have several concurrent processes.
 */
class PProcess : public LineInfo {

    public:
      PProcess(ivl_process_type_t t, Statement*st)
      : type_(t), statement_(st) { }

      virtual ~PProcess();

      bool elaborate(Design*des, NetScope*scope) const;

      ivl_process_type_t type() const { return type_; }
      Statement*statement() { return statement_; }

      map<perm_string,PExpr*> attributes;

      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  int i) const;

    private:
      ivl_process_type_t type_;
      Statement*statement_;
};

/*
 * The PProcess is a process, the Statement is the actual action. In
 * fact, the Statement class is abstract and represents all the
 * possible kinds of statements that exist in Verilog.
 */
class Statement : public LineInfo {

    public:
      Statement() { }
      virtual ~Statement() =0;

      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const = 0;
      virtual void mustmodify(set<PExpr*, ExprComparator>& vars, set<perm_string>) const {}
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;

      map<perm_string, PExpr*> attributes;
};
// PAssign, PAssignNB: Assignment statements, including blocking and non-blocking assignments
/*
 * Assignment statements of the various forms are handled by this
 * type. The rvalue is an expression. The lvalue needs to be figured
 * out by the parser as much as possible.
 */
class PAssign_  : public Statement {
    public:
      explicit PAssign_(PExpr*lval, PExpr*ex, bool is_constant);
      explicit PAssign_(PExpr*lval, PExpr*de, PExpr*ex);
      explicit PAssign_(PExpr*lval, PExpr*cnt, PEventStatement*de, PExpr*ex);
      virtual ~PAssign_() =0;

      PExpr* lval() const  { return lval_; }
      PExpr* rval() const  { return rval_; }

    protected:
      NetAssign_* elaborate_lval(Design*, NetScope*scope) const;
      NetExpr* elaborate_rval_(Design*, NetScope*, unsigned lv_width,
			       ivl_variable_type_t type) const;

      PExpr* delay_;
      PEventStatement*event_;
      PExpr* count_;

    protected:
      PExpr* lval_;
      PExpr* rval_;
      bool is_constant_;
};

class PAssign  : public PAssign_ {

    public:
      explicit PAssign(PExpr*lval, PExpr*ex);
      explicit PAssign(PExpr*lval, PExpr*de, PExpr*ex);
      explicit PAssign(PExpr*lval, PExpr*cnt, PEventStatement*de, PExpr*ex);
      explicit PAssign(PExpr*lval, PExpr*ex, bool is_constant);
      ~PAssign();

      virtual void dump(ostream&out, unsigned ind) const;
      int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual void mustmodify(set<PExpr*, ExprComparator>& vars, set<perm_string>) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      void absintp(Predicate&, TypeEnv&) const;
       bool is_checked() const { return checked_; }
      void mark_checked() { checked_ = true; }

    private:
    bool checked_ = false;
};

class PAssignNB  : public PAssign_ {

    public:
      explicit PAssignNB(PExpr*lval, PExpr*ex);
      explicit PAssignNB(PExpr*lval, PExpr*de, PExpr*ex);
      explicit PAssignNB(PExpr*lval, PExpr*cnt, PEventStatement*de, PExpr*ex);
      ~PAssignNB();

      virtual void dump(ostream&out, unsigned ind) const;
      int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual void mustmodify(set<PExpr*, ExprComparator>& vars, set<perm_string>) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      void absintp(Predicate&, TypeEnv&) const;
      bool is_checked() const { return checked_; }
      void mark_checked() { checked_ = true; }

    private:
    bool checked_ = false;
      NetProc*assign_to_memory_(class NetMemory*, PExpr*,
				Design*des, NetScope*scope) const;
};
// PBlock: A block statement, can be sequential or parallel execution
/*
 * A block statement is an ordered list of statements that make up the
 * block. The block can be sequential or parallel, which only affects
 * how the block is interpreted. The parser collects the list of
 * statements before constructing this object, so it knows a priori
 * what is contained.
 */
#include "PScope.h"
class PBlock  : public PScope, public Statement {

    public:
      enum BL_TYPE { BL_SEQ, BL_PAR };

	// If the block has a name, it is a scope and also has a parent.
      explicit PBlock(perm_string n, PScope*parent, BL_TYPE t);
	// If it doesn't have a name, it's not a scope
      explicit PBlock(BL_TYPE t);
      ~PBlock();
      const svector<Statement*>& get_statements() const { return list_; }
      BL_TYPE bl_type() const { return bl_type_; }

      void set_statement(const svector<Statement*>&st);

      virtual void dump(ostream&out, unsigned ind) const;

      int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual void mustmodify(set<PExpr*, ExprComparator>& vars, set<perm_string>) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;

    private:
      const BL_TYPE bl_type_;
      svector<Statement*>list_;
};
// PCallTask: Task call statement
class PCallTask  : public Statement {

    public:
      explicit PCallTask(const pform_name_t&n, const svector<PExpr*>&parms);
      explicit PCallTask(perm_string n, const svector<PExpr*>&parms);
      ~PCallTask();

      const pform_name_t& path() const;

      unsigned nparms() const { return parms_.count(); }

      PExpr*&parm(unsigned idx)
	    { assert(idx < parms_.count());
	      return parms_[idx];
	    }

      PExpr* parm(unsigned idx) const
	    { assert(idx < parms_.count());
	      return parms_[idx];
	    }

      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      NetProc* elaborate_sys(Design*des, NetScope*scope) const;
      NetProc* elaborate_usr(Design*des, NetScope*scope) const;

      pform_name_t path_;
      svector<PExpr*> parms_;
};
// PCase: Case statement
class PCase  : public Statement {

    public:
      struct Item {
	    svector<PExpr*>expr;
	    Statement*stat;
      };

      PCase(NetCase::TYPE, PExpr*ex, svector<Item*>*);
      ~PCase();
      PExpr* get_expr() const { return expr_; }
      const svector<Item*>& get_items() const { return *items_; }
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&, unsigned idx) const;

    private:
      NetCase::TYPE type_;
      PExpr*expr_;

      svector<Item*>*items_;

    private: // not implemented
      PCase(const PCase&);
      PCase& operator= (const PCase&);
};
// PCAssign: Continuous assignment statement
class PCAssign  : public Statement {

    public:
      explicit PCAssign(PExpr*l, PExpr*r);
      ~PCAssign();

      virtual NetCAssign* elaborate(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*lval_;
      PExpr*expr_;
};
// PCondit: If-else conditional statement
class PCondit  : public Statement {

    public:
      PCondit(PExpr*ex, Statement*i, Statement*e);
      ~PCondit();
      PExpr* get_expr() const { return expr_; }
      Statement* get_if() const { return if_; }
      Statement* get_else() const { return else_; }
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual void mustmodify(set<PExpr*, ExprComparator>& vars, set<perm_string>) const;
      void absintp(Predicate& pred, TypeEnv&, bool istrue) const;
      void merge(Predicate&, Predicate&, Predicate&) const;
      bool is_checked() const { return checked_; }
      void mark_checked() { checked_ = true; }


    private:
      PExpr*expr_;
      bool checked_ = false;
      Statement*if_;
      Statement*else_;

    private: // not implemented
      PCondit(const PCondit&);
      PCondit& operator= (const PCondit&);
};
// PDeassign: Deassign statement
class PDeassign  : public Statement {

    public:
      explicit PDeassign(PExpr*l);
      ~PDeassign();

      virtual NetDeassign* elaborate(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*lval_;
};

class PDelayStatement  : public Statement {

    public:
      PDelayStatement(PExpr*d, Statement*st);
      ~PDelayStatement();

      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*delay_;
      Statement*statement_;
};


/*
 * This represents the parsing of a disable <scope> statement.
 */
class PDisable  : public Statement {

    public:
      explicit PDisable(const pform_name_t&sc);
      ~PDisable();

      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      pform_name_t scope_;
};

/*
 * The event statement represents the event delay in behavioral
 * code. It comes from such things as:
 *
 *      @name <statement>;
 *      @(expr) <statement>;
 *      @* <statement>;
 */
class PEventStatement  : public Statement {

    public:

      explicit PEventStatement(const svector<PEEvent*>&ee);
      explicit PEventStatement(PEEvent*ee);
	// Make an @* statement.
      explicit PEventStatement(void);

      ~PEventStatement();

      void set_statement(Statement*st);
      Statement*statement() { return statement_; }
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
	// Call this with a NULL statement only. It is used to print
	// the event expression for inter-assignment event controls.
      virtual void dump_inline(ostream&out) const;
      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;

      void absintp(Predicate&, TypeEnv&) const;
      bool has_aa_term(Design*des, NetScope*scope);

	// This method is used to elaborate, but attach a previously
	// elaborated statement to the event.
      NetProc* elaborate_st(Design*des, NetScope*scope, NetProc*st) const;

      NetProc* elaborate_wait(Design*des, NetScope*scope, NetProc*st) const;

    private:
      svector<PEEvent*>expr_;
      Statement*statement_;
};

ostream& operator << (ostream&o, const PEventStatement&obj);

class PForce  : public Statement {

    public:
      explicit PForce(PExpr*l, PExpr*r);
      ~PForce();

      virtual NetForce* elaborate(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*lval_;
      PExpr*expr_;
};

class PForever : public Statement {
    public:
      explicit PForever(Statement*s);
      ~PForever();

      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      Statement*statement_;
};

class PForStatement  : public Statement {

    public:
      PForStatement(PExpr*n1, PExpr*e1, PExpr*cond,
		    PExpr*n2, PExpr*e2, Statement*st);
      ~PForStatement();

      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;
      bool is_checked() const { return checked_; }
      void mark_checked() { checked_ = true; }
    private:
    bool checked_ =false;
      PExpr* name1_;
      PExpr* expr1_;

      PExpr*cond_;

      PExpr* name2_;
      PExpr* expr2_;

      Statement*statement_;
};

class PNoop  : public Statement {

    public:
      PNoop() { }
      ~PNoop() { }

    virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
    void absintp(Predicate&, TypeEnv&) const;
};

class PRepeat : public Statement {
    public:
      explicit PRepeat(PExpr*expr, Statement*s);
      ~PRepeat();

      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*expr_;
      Statement*statement_;
};

class PRelease  : public Statement {

    public:
      explicit PRelease(PExpr*l);
      ~PRelease();

      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*lval_;
};

/*
 * The PTrigger statement sends a trigger to a named event. Take the
 * name here.
 */
class PTrigger  : public Statement {

    public:
      explicit PTrigger(const pform_name_t&ev);
      ~PTrigger();

      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      pform_name_t event_;
};

class PWhile  : public Statement {

    public:
      PWhile(PExpr*e1, Statement*st);
      ~PWhile();

      virtual NetProc* elaborate(Design*des, NetScope*scope) const;
      virtual void elaborate_scope(Design*des, NetScope*scope) const;
      virtual void elaborate_sig(Design*des, NetScope*scope) const;
      virtual void dump(ostream&out, unsigned ind) const;
      virtual int typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const;
      void absintp(Predicate&, TypeEnv&) const;

    private:
      PExpr*cond_;
      Statement*statement_;
};

#endif

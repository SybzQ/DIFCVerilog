#ifndef __PExpr_H
#define __PExpr_H
/*
 * Copyright (c) 1998-2011 Stephen Williams <steve@icarus.com>
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

# include  <string>
# include  <vector>
# include  <valarray>
# include  "netlist.h"
# include  "verinum.h"
# include  "LineInfo.h"
# include  "pform_types.h"
#include <sstream>
#include <cstring>
#include <stack>
#include <unordered_set>
class Design;
class Module;
class NetNet;
class NetExpr;
class NetScope;
class SecType;
class SecMaxType ;
class SecDownType;
class PCondit;
/*
 * The PExpr class hierarchy supports the description of
 * expressions. The parser can generate expression objects from the
 * source, possibly reducing things that it knows how to reduce.
 */

class PExpr : public LineInfo {

    public:
      PExpr();
      virtual ~PExpr();

      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const = 0;
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const = 0;
      // This method tests whether the expression contains any
        // references to automatically allocated variables.
      virtual bool has_aa_term(Design*des, NetScope*scope) const;
      virtual bool contains_expr(perm_string that) { return get_name() == that;};
      virtual bool is_wellformed(set<perm_string> s) {return false;};
      virtual PExpr* to_wellformed(set<perm_string> s) {return NULL;};
      virtual bool is_neg_wellformed(set<perm_string> s) {return is_wellformed(s);};
      virtual PExpr* neg_to_wellformed(set<perm_string> s) {return to_wellformed(s);};
      virtual PExpr* subst(map<perm_string, perm_string> m) {return this;}
      virtual PExpr* clone() const = 0;
      virtual bool semantically_equals(const PExpr* other) const {
        return this == other; // Default implementation: pointer equality
    }

    // Semantic hash for fast comparison
    virtual size_t semantic_hash() const {
        return reinterpret_cast<size_t>(this);
    }

    // Lightweight semantic containment check
    virtual bool semantically_contains(const PExpr* target) const {
        return semantically_equals(target);
    }
      // Virtual function to convert a PExpr object to a string
      virtual std::string toString() const {
          std::ostringstream oss;
          dump(oss); // Use the existing dump function to write info to ostringstream
          return oss.str(); // Convert ostringstream to std::string and return
      }

	// This method tests the width that the expression wants to
	// be. It is used by elaboration of assignments to figure out
	// the width of the expression.
	//
	// The "min" is the width of the local context, so is the
	// minimum width that this function should return. Initially
	// this is the same as the lval width.
	//
	// The "lval" is the width of the destination where this
	// result is going to go. This can be used to constrain the
	// amount that an expression can reasonably expand. For
	// example, there is no point expanding an addition to beyond
	// the lval. This extra bit of information allows the
	// expression to optimize itself a bit. If the lval==0, then
	// the subexpression should not make l-value related
	// optimizations.
	//
	// The expr_type is an output argument that gives the
	// calculated type for the expression.
	//
	// The unsized_flag is set to true if the expression is
	// unsized and therefore expandable. This happens if a
	// sub-expression is an unsized literal. Some expressions make
	// special use of that.
      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

	// After the test_width method is complete, these methods
	// return valid results.
      ivl_variable_type_t expr_type() const { return expr_type_; }
      unsigned expr_width() const           { return expr_width_; }
      const perm_string get_name() const;
	// During the elaborate_sig phase, we may need to scan
	// expressions to find implicit net declarations.
      virtual bool elaborate_sig(Design*des, NetScope*scope) const;

	// Procedural elaboration of the expression. The expr_width is
	// the width of the context of the expression (i.e. the
	// l-value width of an assignment),
	//
	// ... or -1 if the expression is self-determined. or
	// ... or -2 if the expression is losslessly
	// self-determined. This can happen in situations where the
	// result is going to a pseudo-infinitely wide context.
	//
	// The sys_task_arg flag is true if expressions are allowed to
	// be incomplete.
      virtual NetExpr*elaborate_expr(Design*des, NetScope*scope,
				     int expr_width, bool sys_task_arg) const;

	// Elaborate expressions that are the r-value of parameter
	// assignments. This elaboration follows the restrictions of
	// constant expressions and supports later overriding and
	// evaluation of parameters.
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

	// This method elaborates the expression as gates, but
	// restricted for use as l-values of continuous assignments.
      virtual NetNet* elaborate_lnet(Design*des, NetScope*scope) const;

	// This is similar to elaborate_lnet, except that the
	// expression is evaluated to be bi-directional. This is
	// useful for arguments to inout ports of module instances and
	// ports of tran primitives.
      virtual NetNet* elaborate_bi_net(Design*des, NetScope*scope) const;

	// Expressions that can be in the l-value of procedural
	// assignments can be elaborated with this method. If the
	// is_force flag is true, then the set of valid l-value types
	// is slightly modified to accommodate the Verilog force
	// statement
      virtual NetAssign_* elaborate_lval(Design*des,
					 NetScope*scope,
					 bool is_force) const;

	// This attempts to evaluate a constant expression, and return
	// a verinum as a result. If the expression cannot be
	// evaluated, return 0.
      virtual verinum* eval_const(Design*des, NetScope*sc) const;

	// This method returns true if the expression represents a
        // structural net that can have multiple drivers. This is
        // used to test whether an input port connection can be
        // collapsed to a single wire.
      virtual bool is_collapsible_net(Design*des, NetScope*scope) const;

	// This method returns true if that expression is the same as
	// this expression. This method is used for comparing
	// expressions that must be structurally "identical".
      virtual bool is_the_same(const PExpr*that) const;

    protected:
	// The derived class test_width methods should fill these in.
      ivl_variable_type_t expr_type_;
      unsigned expr_width_;
        // Helper: compare two sub-expressions for semantic equality
    static bool semantically_equal_exprs(const PExpr* a, const PExpr* b) {
        if (a == b) return true;
        if (!a || !b) return false;
        return a->semantically_equals(b);
    }
    private: // not implemented
      PExpr(const PExpr&);
      PExpr& operator= (const PExpr&);
};

ostream& operator << (ostream&, const PExpr&);

class ExprComparator
{
public:
    bool operator()(const PExpr* e1, const PExpr* e2) const
    {
        return e1->get_name() < e2->get_name();
    }
};

class PEConcat : public PExpr {

    public:
      PEConcat(const svector<PExpr*>&p, PExpr*r =0);
      //~PEConcat();
      ~PEConcat() {
        for (unsigned i = 0; i < parms_.count(); i++) {
            delete parms_[i];
        }
        delete repeat_;
    }
     virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        std::vector<std::string> ret;
        for (unsigned i = 0; i < parms_.count(); i++) {
            if (parms_[i]) {
                auto results = parms_[i]->typecheckstring(varsToType);
                ret.insert(ret.end(), results.begin(), results.end());
            }
        }
        return ret;
    }
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_concat = dynamic_cast<const PEConcat*>(other)) {
            if (parms_.count() != other_concat->parms_.count()) return false;
            for (unsigned i = 0; i < parms_.count(); i++) {
                if (!semantically_equal_exprs(parms_[i], other_concat->parms_[i])) {
                    return false;
                }
            }
            return semantically_equal_exprs(repeat_, other_concat->repeat_);
        }
        return false;
    }

    virtual bool semantically_contains(const PExpr* target) const override {
        if (semantically_equals(target)) return true;
        for (unsigned i = 0; i < parms_.count(); i++) {
            if (parms_[i] && parms_[i]->semantically_contains(target)) {
                return true;
            }
        }
        return repeat_ && repeat_->semantically_contains(target);
    }
      virtual verinum* eval_const(Design*des, NetScope*sc) const;
      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual bool has_aa_term(Design*des, NetScope*scope) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual bool elaborate_sig(Design*des, NetScope*scope) const;
      virtual NetNet* elaborate_lnet(Design*des, NetScope*scope) const;
      virtual NetNet* elaborate_bi_net(Design*des, NetScope*scope) const;
      virtual NetExpr*elaborate_expr(Design*des, NetScope*,
				     int expr_width, bool sys_task_arg) const;
      virtual NetEConcat*elaborate_pexpr(Design*des, NetScope*) const;
      virtual NetAssign_* elaborate_lval(Design*des,
					 NetScope*scope,
					 bool is_force) const;
      virtual bool is_collapsible_net(Design*des, NetScope*scope) const;

      // Convert parms_ to std::vector<std::string> format
      std::vector<std::string> getParmsAsStrings() const {
          std::vector<std::string> strings;
          for (unsigned idx = 0; idx < parms_.count(); idx += 1) {
              strings.push_back(parms_[idx]->get_name().str());
          }
          return strings;
      }
      svector<PExpr*>parms_;
       // Add clone method
    virtual PEConcat* clone() const override {
    // Create new parameter list
    svector<PExpr*> new_parms(parms_.count());
    for (unsigned i = 0; i < parms_.count(); i++) {
        new_parms[i] = parms_[i] ? parms_[i]->clone() : nullptr;
    }

    // Create new repeat expression
    PExpr* new_repeat = repeat_ ? repeat_->clone() : nullptr;

    // Use constructor to create new object
    PEConcat* result = new PEConcat(new_parms, new_repeat);

    // Copy state
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    result->tested_widths_ = tested_widths_;

    return result;
}
    private:
      NetNet* elaborate_lnet_common_(Design*des, NetScope*scope,
				     bool bidirectional_flag) const;
    private:

      std::valarray<unsigned>tested_widths_;

      PExpr*repeat_;
};

/*
 * Event expressions are expressions that can be combined with the
 * event "or" operator. These include "posedge foo" and similar, and
 * also include named events. "edge" events are associated with an
 * expression, whereas named events simply have a name, which
 * represents an event variable.
 */
class PEEvent : public PExpr {

    public:
      enum edge_t {ANYEDGE, POSEDGE, NEGEDGE, POSITIVE};

	// Use this constructor to create events based on edges or levels.
      PEEvent(edge_t t, PExpr*e);

      // Destructor
    ~PEEvent() {
        delete expr_;
    }
       // Add clone method
    virtual PEEvent* clone() const override {
    PExpr* new_expr = expr_ ? expr_->clone() : nullptr;
    PEEvent* result = new PEEvent(type_, new_expr);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
      edge_t type() const;
      PExpr* expr() const;
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        if (expr_) {
            return expr_->typecheckstring(varsToType);
        }
        return {};
    }
      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual bool has_aa_term(Design*des, NetScope*scope) const;

    private:
      edge_t type_;
      PExpr *expr_;
};

/*
 * This holds a floating point constant in the source.
 */
class PEFNumber : public PExpr {

    public:
      explicit PEFNumber(verireal*vp);
      // Destructor
    ~PEFNumber() {
        delete value_;
    }
        virtual PEFNumber* clone() const override {
    verireal* new_value = value_ ? new verireal(*value_) : nullptr;
    PEFNumber* result = new PEFNumber(new_value);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}

    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_float = dynamic_cast<const PEFNumber*>(other)) {
            if (!value_ || !other_float->value_) {
                return !value_ && !other_float->value_; // both null means equal
            }

            // Use as_double() for floating-point comparison, accounting for precision
            return std::abs(value_->as_double() - other_float->value_->as_double()) < 1e-10;
        }
        return false;
    }
      const verireal& value() const;
    virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        // Floating-point constant, no variable name
        return {};
    }
	/* The eval_const method as applied to a floating point number
	   gets the *integer* value of the number. This accounts for
	   any rounding that is needed to get the value. */
      virtual verinum* eval_const(Design*des, NetScope*sc) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);
      virtual NetExpr*elaborate_expr(Design*des, NetScope*,
				     int expr_width, bool sys_task_arg) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

    private:
      verireal*value_;
};

class PEIdent : public PExpr {

    public:
      explicit PEIdent(perm_string);
      explicit PEIdent(const pform_name_t&);
      ~PEIdent();
    // Add clone method
    virtual PEIdent* clone() const override {
    PEIdent* result = new PEIdent(path_);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
    std::vector<PExpr*> get_index_expressions() const {
        std::vector<PExpr*> indices;
        for (const auto& name_component : path_) {
            for (const auto& index_component : name_component.index) {
                switch (index_component.sel) {
                    case index_component_t::SEL_BIT:
                        if (index_component.msb) indices.push_back(index_component.msb);
                        break;
                    case index_component_t::SEL_PART:
                        if (index_component.msb) indices.push_back(index_component.msb);
                        if (index_component.lsb) indices.push_back(index_component.lsb);
                        break;
                    case index_component_t::SEL_IDX_UP:
                    case index_component_t::SEL_IDX_DO:
                        if (index_component.msb) indices.push_back(index_component.msb);
                        break;
                    case index_component_t::SEL_NONE:
                        break;
                }
            }
        }
        return indices;
    }
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        /*if (auto other_ident = dynamic_cast<const PEIdent*>(other)) {
           // return toString() == other_ident->toString();
            //return path_ == other_ident->path_; // compare paths
        return true;
        }*/
        return false;
    }
    // Check if there is an index
    bool has_index() const {
        for (const auto& name_component : path_) {
            if (!name_component.index.empty()) {
                return true;
            }
        }
        return false;
    }
    virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        std::vector<std::string> ret;

        // Add the identifier itself
        //ret.push_back(this->toString());
        std::string base_name = peek_tail_name(path_).str();
        ret.push_back(base_name);
        // Process all index expressions
        auto indices = get_index_expressions();
        for (auto index_expr : indices) {
            if (index_expr) {
                auto index_results = index_expr->typecheckstring(varsToType);
                ret.insert(ret.end(), index_results.begin(), index_results.end());
            }
        }

        return ret;
    }
	// Add another name to the string of hierarchy that is the
	// current identifier.
      void append_name(perm_string);

      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual bool has_aa_term(Design*des, NetScope*scope) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual bool elaborate_sig(Design*des, NetScope*scope) const;

	// Identifiers are allowed (with restrictions) is assign l-values.
      virtual NetNet* elaborate_lnet(Design*des, NetScope*scope) const;

      virtual NetNet* elaborate_bi_net(Design*des, NetScope*scope) const;

	// Identifiers are also allowed as procedural assignment l-values.
      virtual NetAssign_* elaborate_lval(Design*des,
					 NetScope*scope,
					 bool is_force) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*,
				     int expr_width, bool sys_task_arg) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

      virtual bool is_wellformed(set<perm_string> s);
      virtual PExpr* to_wellformed(set<perm_string> s);
      virtual PExpr* subst(map<perm_string, perm_string> m);

	// Elaborate the PEIdent as a port to a module. This method
	// only applies to Ident expressions.
      NetNet* elaborate_port(Design*des, NetScope*sc) const;

      verinum* eval_const(Design*des, NetScope*sc) const;

      virtual bool is_collapsible_net(Design*des, NetScope*scope) const;

      const pform_name_t& path() const { return path_; }

    private:
      pform_name_t path_;

    private:
	// Common functions to calculate parts of part/bit
	// selects. These methods return true if the expressions
	// elaborate/calculate, or false if there is some sort of
	// source error.

	// The calculate_parts_ method calculates the range
	// expressions of a part select for the current object. The
	// part select expressions are elaborated and evaluated, and
	// the values written to the msb/lsb arguments. If there are
	// invalid bits (xz) in either expression, then the defined
	// flag is set to *false*.
      bool calculate_parts_(Design*, NetScope*, long&msb, long&lsb, bool&defined) const;
      NetExpr* calculate_up_do_base_(Design*, NetScope*) const;
      bool calculate_param_range_(Design*, NetScope*,
				  const NetExpr*msb_ex, long&msb,
				  const NetExpr*lsb_ex, long&lsb,
				  long length) const;

      bool calculate_up_do_width_(Design*, NetScope*, unsigned long&wid) const;

    private:
      NetAssign_*elaborate_lval_net_word_(Design*, NetScope*, NetNet*) const;
      bool elaborate_lval_net_bit_(Design*, NetScope*, NetAssign_*) const;
      bool elaborate_lval_net_part_(Design*, NetScope*, NetAssign_*) const;
      bool elaborate_lval_net_idx_(Design*, NetScope*, NetAssign_*,
                                   index_component_t::ctype_t) const;

    private:
      NetExpr*elaborate_expr_param_(Design*des,
				    NetScope*scope,
				    const NetExpr*par,
				    NetScope*found,
				    const NetExpr*par_msb,
				    const NetExpr*par_lsb,
				    int expr_wid) const;
      NetExpr*elaborate_expr_param_part_(Design*des,
					 NetScope*scope,
					 const NetExpr*par,
					 NetScope*found,
					 const NetExpr*par_msb,
					 const NetExpr*par_lsb) const;
      NetExpr*elaborate_expr_param_idx_up_(Design*des,
					   NetScope*scope,
					   const NetExpr*par,
					   NetScope*found,
					   const NetExpr*par_msb,
					   const NetExpr*par_lsb) const;
      NetExpr*elaborate_expr_param_idx_do_(Design*des,
					   NetScope*scope,
					   const NetExpr*par,
					   NetScope*found,
					   const NetExpr*par_msb,
					   const NetExpr*par_lsb) const;
      NetExpr*elaborate_expr_net(Design*des,
				 NetScope*scope,
				 NetNet*net,
				 NetScope*found,
				 bool sys_task_arg) const;
      NetExpr*elaborate_expr_net_word_(Design*des,
				       NetScope*scope,
				       NetNet*net,
				       NetScope*found,
				       bool sys_task_arg) const;
      NetExpr*elaborate_expr_net_part_(Design*des,
				   NetScope*scope,
				   NetESignal*net,
				   NetScope*found) const;
      NetExpr*elaborate_expr_net_idx_up_(Design*des,
				   NetScope*scope,
				   NetESignal*net,
				   NetScope*found) const;
      NetExpr*elaborate_expr_net_idx_do_(Design*des,
				   NetScope*scope,
				   NetESignal*net,
				   NetScope*found) const;
      NetExpr*elaborate_expr_net_bit_(Design*des,
				   NetScope*scope,
				   NetESignal*net,
				   NetScope*found) const;

    private:
      NetNet* elaborate_lnet_common_(Design*des, NetScope*scope,
				     bool bidirectional_flag) const;

      NetNet*make_implicit_net_(Design*des, NetScope*scope) const;

      bool eval_part_select_(Design*des, NetScope*scope, NetNet*sig,
			     long&midx, long&lidx) const;
};

class PENumber : public PExpr {

    public:
      explicit PENumber(verinum*vp);
      //~PENumber();
        // Destructor
    ~PENumber() {
        delete value_;
    }
    // Add clone method
    virtual PENumber* clone() const override {
    verinum* new_value = value_ ? new verinum(*value_) : nullptr;
    PENumber* result = new PENumber(new_value);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}

      const verinum& value() const;
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        // Numeric constant, no variable name
        return {};
    }
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_num = dynamic_cast<const PENumber*>(other)) {
            return value_ && other_num->value_ && *value_ == *other_num->value_;
        }
        return false;
    }
      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual NetEConst*elaborate_expr(Design*des, NetScope*,
				       int expr_width, bool) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual NetAssign_* elaborate_lval(Design*des,
					 NetScope*scope,
					 bool is_force) const;

      virtual verinum* eval_const(Design*des, NetScope*sc) const;

      virtual bool is_the_same(const PExpr*that) const;
      virtual bool is_wellformed(set<perm_string> s);
      virtual PExpr* to_wellformed(set<perm_string> s);

    private:
      verinum*const value_;
};

/*
 * This represents a string constant in an expression.
 *
 * The s parameter to the PEString constructor is a C string that this
 * class instance will take for its own. The caller should not delete
 * the string, the destructor will do it.
 */
class PEString : public PExpr {

    public:
      explicit PEString(char*s);
      ~PEString();
      // Add clone method
    virtual PEString* clone() const override {
    char* new_text = nullptr;
    if (text_) {
        new_text = new char[strlen(text_) + 1];
        strcpy(new_text, text_);
    }
    PEString* result = new PEString(new_text);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        // String constant, no variable name
        return {};
    }
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_str = dynamic_cast<const PEString*>(other)) {
            return text_ && other_str->text_ && strcmp(text_, other_str->text_) == 0;
        }
        return false;
    }
      string value() const;
      virtual void dump(ostream&) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual NetEConst*elaborate_expr(Design*des, NetScope*,
				       int expr_width, bool) const;
      virtual NetEConst*elaborate_pexpr(Design*des, NetScope*sc) const;
      verinum* eval_const(Design*, NetScope*) const;

    private:
      char*text_;
};

class PEUnary : public PExpr {

    public:
      explicit PEUnary(char op, PExpr*ex);
      //~PEUnary();
      // Destructor
    ~PEUnary() {
        delete expr_;
    }
     // Add clone method
    virtual PEUnary* clone() const override {
    PExpr* new_expr = expr_ ? expr_->clone() : nullptr;
    PEUnary* result = new PEUnary(op_, new_expr);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}

      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        if (expr_) {
            return expr_->typecheckstring(varsToType);
        }
        return {};
    }
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_unary = dynamic_cast<const PEUnary*>(other)) {
            return op_ == other_unary->op_ &&
                   semantically_equal_exprs(expr_, other_unary->expr_);
        }
        return false;
    }

    virtual bool semantically_contains(const PExpr* target) const override {
        return semantically_equals(target) ||
               (expr_ && expr_->semantically_contains(target));
    }
      virtual void dump(ostream&out) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual bool has_aa_term(Design*des, NetScope*scope) const;
      virtual bool contains_expr(perm_string that);

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual bool elaborate_sig(Design*des, NetScope*scope) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*,
				     int expr_width, bool sys_task_arg) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(Design*des, NetScope*sc) const;
      virtual bool is_wellformed(set<perm_string> s);
      virtual PExpr* to_wellformed(set<perm_string> s);
      virtual PExpr* subst(map<perm_string, perm_string> m);
      PExpr*expr_;
    private:
      NetExpr* elaborate_expr_bits_(NetExpr*operand, int expr_wid) const;

    private:
      char op_;

};

class PEBinary : public PExpr {

    public:
      explicit PEBinary(char op, PExpr*l, PExpr*r);
      //~PEBinary();
      ~PEBinary() {
        delete left_;
        delete right_;
    }
      // Add clone method
    virtual PEBinary* clone() const override {
    PExpr* new_left = left_ ? left_->clone() : nullptr;
    PExpr* new_right = right_ ? right_->clone() : nullptr;
    PEBinary* result = new PEBinary(op_, new_left, new_right);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}

    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_binary = dynamic_cast<const PEBinary*>(other)) {
            return op_ == other_binary->op_ &&
                   semantically_equal_exprs(left_, other_binary->left_) &&
                   semantically_equal_exprs(right_, other_binary->right_);
        }
        return false;
    }

    virtual bool semantically_contains(const PExpr* target) const override {
        return semantically_equals(target) ||
               (left_ && left_->semantically_contains(target)) ||
               (right_ && right_->semantically_contains(target));
    }
      virtual void dump(ostream&out) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;
    /*  virtual std::vector<std::string> typecheckstring( map<perm_string, SecType*>& varsToType) const
      {
          std::vector<std::string> ret;
          if (dynamic_cast<PEBinary*>(left_) != nullptr) {

              PEBinary* peconcat = dynamic_cast<PEBinary*>(left_);
              std::vector<std::string> leftResults = peconcat->typecheckstring( varsToType);
              ret.insert(ret.end(), leftResults.begin(), leftResults.end());
          }
          else if (dynamic_cast<PEConcat*>(left_) != nullptr)
          {
              PEConcat* peconcat = dynamic_cast<PEConcat*>(left_);
              std::vector<std::string> leftResults = peconcat->getParmsAsStrings();
              ret.insert(ret.end(), leftResults.begin(), leftResults.end());
          }
          else ret.push_back(left_->toString());

          if (dynamic_cast<PEBinary*>(right_) != nullptr) {
              PEBinary* peconcat = dynamic_cast<PEBinary*>(right_);
              std::vector<std::string> rightResults = peconcat->typecheckstring( varsToType);
              ret.insert(ret.end(), rightResults.begin(), rightResults.end());
          }
          else if (dynamic_cast<PEConcat*>(right_) != nullptr)
          {
              PEConcat* peconcat = dynamic_cast<PEConcat*>(right_);
              std::vector<std::string> rightResults = peconcat->getParmsAsStrings();
              ret.insert(ret.end(), rightResults.begin(), rightResults.end());
          }
          else ret.push_back(right_->toString());

          return ret;

      }*/
    /* virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const
{
    std::vector<std::string> ret;

    // Process left subtree
    if (dynamic_cast<PEBinary*>(left_) != nullptr) {
        PEBinary* peconcat = dynamic_cast<PEBinary*>(left_);
        std::vector<std::string> leftResults = peconcat->typecheckstring(varsToType);
        ret.insert(ret.end(), leftResults.begin(), leftResults.end());
    }
    else if (dynamic_cast<PEConcat*>(left_) != nullptr) {
        PEConcat* peconcat = dynamic_cast<PEConcat*>(left_);
        std::vector<std::string> leftResults = peconcat->getParmsAsStrings();
        ret.insert(ret.end(), leftResults.begin(), leftResults.end());
    }
    else if (auto ident = dynamic_cast<PEIdent*>(left_)) {
        // Process identifier index expressions
        ret.push_back(ident->toString());
        if (ident->has_index()) {
            auto indices = ident->get_index_expressions();
            for (auto index_expr : indices) {
                // Recursively process index expressions
                if (auto binary = dynamic_cast<PEBinary*>(index_expr)) {
                    auto index_results = binary->typecheckstring(varsToType);
                    ret.insert(ret.end(), index_results.begin(), index_results.end());
                } else if (auto concat = dynamic_cast<PEConcat*>(index_expr)) {
                    auto index_results = concat->getParmsAsStrings();
                    ret.insert(ret.end(), index_results.begin(), index_results.end());
                } else {
                    ret.push_back(index_expr->toString());
                }
            }
        } else {
            ret.push_back(left_->toString());
        }
    }
    else {
        ret.push_back(left_->toString());
    }

    // Process right subtree
    if (dynamic_cast<PEBinary*>(right_) != nullptr) {
        PEBinary* peconcat = dynamic_cast<PEBinary*>(right_);
        std::vector<std::string> rightResults = peconcat->typecheckstring(varsToType);
        ret.insert(ret.end(), rightResults.begin(), rightResults.end());
    }
    else if (dynamic_cast<PEConcat*>(right_) != nullptr) {
        PEConcat* peconcat = dynamic_cast<PEConcat*>(right_);
        std::vector<std::string> rightResults = peconcat->getParmsAsStrings();
        ret.insert(ret.end(), rightResults.begin(), rightResults.end());
    }
    else if (auto ident = dynamic_cast<PEIdent*>(right_)) {
        // Process identifier index expressions
        ret.push_back(ident->toString());
        if (ident->has_index()) {
            auto indices = ident->get_index_expressions();
            for (auto index_expr : indices) {
                // Recursively process index expressions
                if (auto binary = dynamic_cast<PEBinary*>(index_expr)) {
                    auto index_results = binary->typecheckstring(varsToType);
                    ret.insert(ret.end(), index_results.begin(), index_results.end());
                } else if (auto concat = dynamic_cast<PEConcat*>(index_expr)) {
                    auto index_results = concat->getParmsAsStrings();
                    ret.insert(ret.end(), index_results.begin(), index_results.end());
                } else {
                    ret.push_back(index_expr->toString());
                }
            }
        } else {
            ret.push_back(right_->toString());
        }
    }
    else {
        ret.push_back(right_->toString());
    }

    return ret;
}*/
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        std::vector<std::string> ret;

        if (left_) {
            auto left_results = left_->typecheckstring(varsToType);
            ret.insert(ret.end(), left_results.begin(), left_results.end());
        }

        if (right_) {
            auto right_results = right_->typecheckstring(varsToType);
            ret.insert(ret.end(), right_results.begin(), right_results.end());
        }

        return ret;
    }


      virtual bool has_aa_term(Design*des, NetScope*scope) const;
      virtual bool contains_expr(perm_string that);

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual bool elaborate_sig(Design*des, NetScope*scope) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*,
					int expr_width, bool sys_task_arg) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(Design*des, NetScope*sc) const;
      virtual bool is_wellformed(set<perm_string> s);
      virtual PExpr* to_wellformed(set<perm_string> s);
      virtual bool is_neg_wellformed(set<perm_string> s);
      virtual PExpr* neg_to_wellformed(set<perm_string> s);
      virtual PExpr* subst(map<perm_string, perm_string> m);

      PExpr* left_;
      PExpr* right_;

    protected:
      char op_;


      NetExpr*elaborate_expr_base_(Design*, NetExpr*lp, NetExpr*rp,
				   int use_wid, bool is_pexpr =false) const;
      NetExpr*elaborate_eval_expr_base_(Design*, NetExpr*lp, NetExpr*rp,
					int use_wid) const;

      NetExpr*elaborate_expr_base_bits_(Design*, NetExpr*lp, NetExpr*rp, int use_wid) const;
      NetExpr*elaborate_expr_base_div_(Design*, NetExpr*lp, NetExpr*rp,
				       int use_wid, bool is_pexpr) const;
      NetExpr*elaborate_expr_base_lshift_(Design*, NetExpr*lp, NetExpr*rp, int use_wid) const;
      NetExpr*elaborate_expr_base_rshift_(Design*, NetExpr*lp, NetExpr*rp, int use_wid) const;
      NetExpr*elaborate_expr_base_mult_(Design*, NetExpr*lp, NetExpr*rp,
					int use_wid, bool is_pexpr) const;
      NetExpr*elaborate_expr_base_add_(Design*, NetExpr*lp, NetExpr*rp,
				       int use_wid, bool is_pexpr) const;

};

/*
 * Here are a few specialized classes for handling specific binary
 * operators.
 */
class PEBComp  : public PEBinary {

    public:
      explicit PEBComp(char op, PExpr*l, PExpr*r);
      ~PEBComp();
      virtual PEBComp* clone() const override {
    PExpr* new_left = left_ ? left_->clone() : nullptr;
    PExpr* new_right = right_ ? right_->clone() : nullptr;
    PEBComp* result = new PEBComp(op_, new_left, new_right);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    result->left_width_ = left_width_;
    result->right_width_ = right_width_;
    return result;
}


      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&flag);

      NetExpr* elaborate_expr(Design*des, NetScope*scope,
			      int expr_width, bool sys_task_arg) const;
      NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

    private:
      int left_width_;
      int right_width_;
};

/*
 * This derived class is for handling logical expressions: && and ||.
*/
class PEBLogic  : public PEBinary {

    public:
      explicit PEBLogic(char op, PExpr*l, PExpr*r);
      ~PEBLogic();
     virtual PEBLogic* clone() const override {
    PExpr* new_left = left_ ? left_->clone() : nullptr;
    PExpr* new_right = right_ ? right_->clone() : nullptr;
    PEBLogic* result = new PEBLogic(op_, new_left, new_right);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&flag);

      NetExpr* elaborate_expr(Design*des, NetScope*scope,
			      int expr_width, bool sys_task_arg) const;
      NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;
};

/*
 * A couple of the binary operands have a special sub-expression rule
 * where the expression width is carried entirely by the left
 * expression, and the right operand is self-determined.
 */
class PEBLeftWidth  : public PEBinary {

    public:
      explicit PEBLeftWidth(char op, PExpr*l, PExpr*r);
      ~PEBLeftWidth() =0;
     virtual PEBLeftWidth* clone() const override = 0;
      virtual NetExpr*elaborate_expr_leaf(Design*des, NetExpr*lp, NetExpr*rp,
					  int expr_wid) const =0;

    protected:
      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&flag);

      virtual NetExpr*elaborate_expr(Design*des, NetScope*scope,
				     int expr_width, bool sys_task_arg) const;

      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*scope) const;

};

class PEBPower  : public PEBLeftWidth {

    public:
      explicit PEBPower(char op, PExpr*l, PExpr*r);
      ~PEBPower();
      // Use parent class clone method
    virtual PEBPower* clone() const override {
    PExpr* new_left = left_ ? left_->clone() : nullptr;
    PExpr* new_right = right_ ? right_->clone() : nullptr;
    PEBPower* result = new PEBPower(op_, new_left, new_right);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
      NetExpr*elaborate_expr_leaf(Design*des, NetExpr*lp, NetExpr*rp,
				  int expr_wid) const;
};

class PEBShift  : public PEBLeftWidth {

    public:
      explicit PEBShift(char op, PExpr*l, PExpr*r);
      ~PEBShift();
    virtual PEBShift* clone() const override {
    PExpr* new_left = left_ ? left_->clone() : nullptr;
    PExpr* new_right = right_ ? right_->clone() : nullptr;
    PEBShift* result = new PEBShift(op_, new_left, new_right);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
      NetExpr*elaborate_expr_leaf(Design*des, NetExpr*lp, NetExpr*rp,
				  int expr_wid) const;
};

/*
 * This class supports the ternary (?:) operator. The operator takes
 * three expressions, the test, the true result and the false result.
 */
class PETernary : public PExpr {

    public:
      explicit PETernary(PExpr*e, PExpr*t, PExpr*f);
      //~PETernary();
      // Destructor
    ~PETernary() {
        delete expr_;
        delete tru_;
        delete fal_;
    }
    virtual PETernary* clone() const override {
    PExpr* new_expr = expr_ ? expr_->clone() : nullptr;
    PExpr* new_tru = tru_ ? tru_->clone() : nullptr;
    PExpr* new_fal = fal_ ? fal_->clone() : nullptr;
    PETernary* result = new PETernary(new_expr, new_tru, new_fal);
    result->expr_type_ = expr_type_;
    result->expr_width_ = expr_width_;
    return result;
}
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        if (auto other_ternary = dynamic_cast<const PETernary*>(other)) {
            return semantically_equal_exprs(expr_, other_ternary->expr_) &&
                   semantically_equal_exprs(tru_, other_ternary->tru_) &&
                   semantically_equal_exprs(fal_, other_ternary->fal_);
        }
        return false;
    }

    virtual bool semantically_contains(const PExpr* target) const override {
        return semantically_equals(target) ||
               (expr_ && expr_->semantically_contains(target)) ||
               (tru_ && tru_->semantically_contains(target)) ||
               (fal_ && fal_->semantically_contains(target));
    }
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        std::vector<std::string> ret;

        if (expr_) {
            auto expr_results = expr_->typecheckstring(varsToType);
            ret.insert(ret.end(), expr_results.begin(), expr_results.end());
        }

        if (tru_) {
            auto tru_results = tru_->typecheckstring(varsToType);
            ret.insert(ret.end(), tru_results.begin(), tru_results.end());
        }

        if (fal_) {
            auto fal_results = fal_->typecheckstring(varsToType);
            ret.insert(ret.end(), fal_results.begin(), fal_results.end());
        }

        return ret;
    }
      virtual void dump(ostream&out) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual bool has_aa_term(Design*des, NetScope*scope) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);

      virtual bool elaborate_sig(Design*des, NetScope*scope) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*,
					 int expr_width, bool sys_task_arg) const;
      virtual NetETernary*elaborate_pexpr(Design*des, NetScope*sc) const;
      virtual verinum* eval_const(Design*des, NetScope*sc) const;
      PCondit* translate (PExpr* lhs);

    private:
      NetExpr* elab_and_eval_alternative_(Design*des, NetScope*scope,
					  PExpr*expr, int use_wid) const;

    public:
      PExpr*expr_;
      PExpr*tru_;
      PExpr*fal_;
};

/*
 * This class represents a parsed call to a function, including calls
 * to system functions. The parameters in the parms list are the
 * expressions that are passed as input to the ports of the function.
 */
class PECallFunction : public PExpr {
    public:
      explicit PECallFunction(const pform_name_t&n, const vector<PExpr *> &parms);
	// Call of system function (name is not hierarchical)
      explicit PECallFunction(perm_string n, const vector<PExpr *> &parms);
      explicit PECallFunction(perm_string n);

	// svector versions. Should be removed!
      explicit PECallFunction(const pform_name_t&n, const svector<PExpr *> &parms);
      explicit PECallFunction(perm_string n, const svector<PExpr *> &parms);
      // Add clone method
     virtual PExpr* clone() const override {
        vector<PExpr*> new_parms;
        for (auto param : parms_) {
            new_parms.push_back(param ? param->clone() : nullptr);
        }
        PECallFunction* result = new PECallFunction(path_, new_parms);
        result->expr_type_ = expr_type_;
        result->expr_width_ = expr_width_;
        return result;
    }
     // ~PECallFunction();
      ~PECallFunction() {
        for (auto param : parms_) {
            delete param;
        }
    }
    virtual bool semantically_equals(const PExpr* other) const override {
        if (this == other) return true;
        /*if (auto other_call = dynamic_cast<const PECallFunction*>(other)) {
            if (path_ != other_call->path_) return false;
            if (parms_.size() != other_call->parms_.size()) return false;
            for (size_t i = 0; i < parms_.size(); i++) {
                if (!semantically_equal_exprs(parms_[i], other_call->parms_[i])) {
                    return false;
                }
            }
            return true;
        }*/
        return false;
    }

    virtual bool semantically_contains(const PExpr* target) const override {
        if (semantically_equals(target)) return true;
        for (auto param : parms_) {
            if (param && param->semantically_contains(target)) {
                return true;
            }
        }
        return false;
    }
      virtual std::vector<std::string> typecheckstring(map<perm_string, SecType*>& varsToType) const override {
        std::vector<std::string> ret;

        // Process all parameters
        for (auto param : parms_) {
            if (param) {
                auto param_results = param->typecheckstring(varsToType);
                ret.insert(ret.end(), param_results.begin(), param_results.end());
            }
        }

        return ret;
    }
      virtual void dump(ostream &) const;
      virtual SecType* typecheck(ostream&out, map<perm_string, SecType*>&varsToType) const;

      virtual bool has_aa_term(Design*des, NetScope*scope) const;

      virtual NetExpr*elaborate_expr(Design*des, NetScope*scope,
				     int expr_wid, bool sys_task_arg) const;
      virtual NetExpr*elaborate_pexpr(Design*des, NetScope*sc) const;

      virtual unsigned test_width(Design*des, NetScope*scope,
				  unsigned min, unsigned lval,
				  ivl_variable_type_t&expr_type,
				  bool&unsized_flag);
      vector<PExpr *> parms_;
    private:
      pform_name_t path_;


      bool check_call_matches_definition_(Design*des, NetScope*dscope) const;

      NetExpr* cast_to_width_(NetExpr*expr, int wid, bool signed_flag) const;

      NetExpr* elaborate_sfunc_(Design*des, NetScope*scope, int expr_wid) const;
      NetExpr* elaborate_access_func_(Design*des, NetScope*scope, ivl_nature_t) const;
      unsigned test_width_sfunc_(Design*des, NetScope*scope,
				 unsigned min, unsigned lval,
				 ivl_variable_type_t&expr_type,
				 bool&unsized_flag);
};

#endif

#ifndef __PEXPR_UTILS_H    // If __PEXPR_UTILS_H is not defined
#define __PEXPR_UTILS_H
// End of PExpr.h

// Helper functions for expression matching
inline bool expr_contains(const PExpr* container, PExpr* target,
                         const std::string& debug_info = "") {
    if (!container || !target) {

          //  cout << "  [expr_contains] Null pointer: container=" << container
               //  << ", target=" << target << " " << debug_info << endl;

        return false;
    }

    // Check literals
    if (dynamic_cast<const PENumber*>(target) || dynamic_cast<const PENumber*>(container) ||
        dynamic_cast<const PEFNumber*>(target) || dynamic_cast<const PEFNumber*>(container) ||
        dynamic_cast<const PEString*>(target) || dynamic_cast<const PEString*>(container)) {

           // cout << "  [expr_contains] Literal detected, returning false " << debug_info << endl;

        return false;
    }

    // Detailed output of expression info

      //  cout << "  [expr_contains] Comparing:" << endl;
       // cout << "    Container: " << container->toString() << " (type: " << typeid(*container).name() << ")" << endl;
      //  cout << "    Target: " << target->toString() << " (type: " << typeid(*target).name() << ")" << endl;
       // cout << "    " << debug_info << endl;


    // Exact match || container->is_the_same(target)
    if (container->toString() == target->toString() ) {

            //cout << "  [expr_contains] Exact match found! " << debug_info << endl;

        return true;
    }

    // String containment check
    std::string container_str = container->toString();
    std::string target_str = target->toString();

    if (container_str.find(target_str) != std::string::npos) {

            cout << "  [expr_contains] String contains: '" << target_str
                 << "' in '" << container_str << "' " << debug_info << endl;

        return true;
    } else {

         //   cout << "  [expr_contains] No string match: '" << target_str
             //    << "' not in '" << container_str << "' " << debug_info << endl;

    }

    // Recursively check sub-expressions
    if (auto unary = dynamic_cast<const PEUnary*>(container)) {
        bool result = expr_contains(unary->expr_, target, debug_info + " -> unary");
        if (result ) {
            cout << "  [expr_contains] Found in unary subexpression " << debug_info << endl;
        }
        return result;
    }
    else if (auto binary = dynamic_cast<const PEBinary*>(container)) {
        bool left_result = expr_contains(binary->left_, target, debug_info + " -> left");
        bool right_result = expr_contains(binary->right_, target, debug_info + " -> right");
        if ((left_result || right_result) ) {
            cout << "  [expr_contains] Found in binary subexpression " << debug_info << endl;
        }
        return left_result || right_result;
    }
    else if (auto ternary = dynamic_cast<const PETernary*>(container)) {
        bool expr_result = expr_contains(ternary->expr_, target, debug_info + " -> ternary_expr");
        bool tru_result = expr_contains(ternary->tru_, target, debug_info + " -> ternary_tru");
        bool fal_result = expr_contains(ternary->fal_, target, debug_info + " -> ternary_fal");
        if ((expr_result || tru_result || fal_result) ) {
            cout << "  [expr_contains] Found in ternary subexpression " << debug_info << endl;
        }
        return expr_result || tru_result || fal_result;
    }
    else if (auto concat = dynamic_cast<const PEConcat*>(container)) {
        for (unsigned i = 0; i < concat->parms_.count(); i++) {
            bool param_result = expr_contains(concat->parms_[i], target,
                                            debug_info + " -> concat[" + to_string(i) + "]");
            if (param_result) {
                    cout << "  [expr_contains] Found in concat parameter " << i << " " << debug_info << endl;

                return true;
            }
        }
    }
    else if (auto call = dynamic_cast<const PECallFunction*>(container)) {
        for (unsigned i = 0; i < call->parms_.size(); i++) {
            bool param_result = expr_contains(call->parms_[i], target,
                                            debug_info + " -> call_param[" + to_string(i) + "]");
            if (param_result) {
                    cout << "  [expr_contains] Found in function call parameter " << i << " " << debug_info << endl;

                return true;
            }
        }
    }
    else if (auto event = dynamic_cast<const PEEvent*>(container)) {
        bool result = expr_contains(event->expr(), target, debug_info + " -> event");
        if (result ) {
            cout << "  [expr_contains] Found in event expression " << debug_info << endl;
        }
        return result;
    }
    else if (dynamic_cast<const PEIdent*>(container)) {
        // PEIdent has no sub-expressions, return false directly
          //  cout << "  [expr_contains] PEIdent has no subexpressions " << debug_info << endl;

    }
    else {
        // Other unhandled expression types
          //  cout << "  [expr_contains] Unhandled expression type: " << typeid(*container).name()
              //   << " " << debug_info << endl;

    }

      //  cout << "  [expr_contains] No match found " << debug_info << endl;

    return false;
}


/*inline bool expr_contains(const PExpr* container,  PExpr* target) {
    if (!container || !target) return false;

    // Fast path: direct equality
    if (container->semantically_equals(target)) return true;

    // Use container's own semantic containment check (if implemented)
    if (container->semantically_contains(target)) return true;

    // Generic iteration check
    std::stack<const PExpr*> stack;
    std::unordered_set<const PExpr*> visited;

    stack.push(container);
    visited.insert(container);

    while (!stack.empty()) {
        const PExpr* current = stack.top();
        stack.pop();

        // Use semantic containment check
        if (current->semantically_contains(target)) {
            return true;
        }

        // Generic sub-expression traversal (fallback)
        if (auto unary = dynamic_cast<const PEUnary*>(current)) {
            if (unary->expr_ && visited.insert(unary->expr_).second) {
                stack.push(unary->expr_);
            }
        }
        else if (auto binary = dynamic_cast<const PEBinary*>(current)) {
            if (binary->left_ && visited.insert(binary->left_).second) {
                stack.push(binary->left_);
            }
            if (binary->right_ && visited.insert(binary->right_).second) {
                stack.push(binary->right_);
            }
        }
        else if (auto ternary = dynamic_cast<const PETernary*>(current)) {
            if (ternary->expr_ && visited.insert(ternary->expr_).second) {
                stack.push(ternary->expr_);
            }
            if (ternary->tru_ && visited.insert(ternary->tru_).second) {
                stack.push(ternary->tru_);
            }
            if (ternary->fal_ && visited.insert(ternary->fal_).second) {
                stack.push(ternary->fal_);
            }
        }
        else if (auto concat = dynamic_cast<const PEConcat*>(current)) {
            for (unsigned i = 0; i < concat->parms_.count(); i++) {
                if (concat->parms_[i] && visited.insert(concat->parms_[i]).second) {
                    stack.push(concat->parms_[i]);
                }
            }
        }
        else if (auto call = dynamic_cast<const PECallFunction*>(current)) {
            for (auto param : call->parms_) {
                if (param && visited.insert(param).second) {
                    stack.push(param);
                }
            }
        }
        else if (auto event = dynamic_cast<const PEEvent*>(current)) {
            auto expr = event->expr();
            if (expr && visited.insert(expr).second) {
                stack.push(expr);
            }
        }
    }

    return false;
}*/





#endif

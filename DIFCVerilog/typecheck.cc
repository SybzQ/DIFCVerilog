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
# include "config.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>
# include  <typeinfo>
# include  <cstdlib>
# include  <sstream>
# include  <fstream>
# include  <algorithm>
# include  "pform.h"
# include  "PEvent.h"
# include  "PGenerate.h"
# include  "PGate.h"
# include  "PSpec.h"
# include  "netlist.h"
# include  "netmisc.h"
# include  "util.h"
# include  "parse_api.h"
# include  "compiler.h"
# include  "ivl_assert.h"
# include  "sectypes.h"
# include <algorithm>
# include <cctype>
# include <iostream>
# include <string>
#include <cstdio>
#include <limits>
#include <cstring>
#include <pthread.h>
#include <stdbool.h>
#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <tuple>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <regex>
#include <queue>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <cctype>
#include <chrono>
#include "GlobalVars.h"
# include  "PExpr.h"

extern bool check_write;


int typecheck_assignment_constraint(ostream& out,
		Predicate pred, string note, string vardecl, TypeEnv* env,  PExpr* lexpr, PExpr* rexpr,int i, std::vector<std::string> rs_tokens, std::vector<std::string> ls_tokens);
int typecheck_assignment(ostream& out, PExpr* lhs, PExpr* rhs, TypeEnv* env,
		Predicate &precond, Predicate postcond, unsigned int lineno, string note,int i, std::string r);


/**
 * Type-check parameters.
 */
void LexicalScope::typecheck_parameters_(ostream& out, TypeEnv& env) const {
		typedef map<perm_string, param_expr_t>::const_iterator parm_iter_t;
		if (debug_typecheck) {
			for (parm_iter_t cur = parameters.begin();
				cur != parameters.end();
				cur++) {
				cout << "check parameter " << (*cur).first << " " << (*cur).second.type << endl;
			}
		}
	for (parm_iter_t cur = parameters.begin(); cur != parameters.end(); cur++) {
    SecType* sec_type = cur->second.get_sec_type();
    if (sec_type == nullptr) {
        if (debug_typecheck) {
            cerr << "Warning: null security type for parameter " << (*cur).first << endl;
        }
        (*env.varsToType)[(*cur).first] = ConstType::BOT;
    } else {
        (*env.varsToType)[(*cur).first] = sec_type;
    }
}
	}


/**
 * Do nothing for now.
 */
void LexicalScope::typecheck_localparams_(ostream&out, TypeEnv& env) const {
	typedef map<perm_string, param_expr_t>::const_iterator parm_iter_t;
	if (debug_typecheck) {
		for (parm_iter_t cur = localparams.begin();
				cur != localparams.end();
				cur++) {
			cout << "check localparam ";
			if ((*cur).second.msb)
				cout << "[" << *(*cur).second.msb << ":" << *(*cur).second.lsb
						<< "] ";
			cout << (*cur).first << " = ";
			if ((*cur).second.expr)
				cout << *(*cur).second.expr << ";" << endl;
			else
				cout << "/* ERROR */;" << endl;
		}
	}
		for (parm_iter_t cur = localparams.begin(); cur != localparams.end(); cur++) {
    SecType* sec_type = cur->second.get_sec_type();
    if (sec_type == nullptr) {
        if (debug_typecheck) {
            cerr << "Warning: null security type for parameter " << (*cur).first << endl;
        }
        (*env.varsToType)[(*cur).first] = ConstType::BOT;
    } else {
        (*env.varsToType)[(*cur).first] = sec_type;
    }
}
}

/**
 * Do nothing for now.
 */
void LexicalScope::typecheck_events_(ostream&out, TypeEnv& env) const {
	if (debug_typecheck) {
		for (map<perm_string, PEvent*>::const_iterator cur = events.begin();
				cur != events.end();
				cur++) {
			PEvent*ev = (*cur).second;
			cout << "check event " << ev->name() << endl;
		}
	}
}

/**
 * Get the security labels for wires.
 */
void LexicalScope::typecheck_wires_(ostream&out, TypeEnv& env) const {

	for (map<perm_string, PWire*>::const_iterator wire = wires.begin();
			wire != wires.end();
			wire++) {
		(*wire).second->typecheck(out, (*env.varsToType));
	}


}


/*
 * Type-check a wire.
 */
void PWire::typecheck(ostream& out, map<perm_string, SecType*>& varsToType) const {
	if (debug_typecheck) {
		cout << "PWire::check " << type_;

		switch (port_type_) {
		case NetNet::PIMPLICIT:
			cout << " implicit input";
			break;
		case NetNet::PINPUT:
			cout << " input";
			break;
		case NetNet::POUTPUT:
			cout << " output";
			break;
		case NetNet::PINOUT:
			cout << " inout";
			break;
		case NetNet::NOT_A_PORT:
			break;
		}


		if (signed_) {
			cout << " signed";
		}

		if (discipline_) {
			cout << " discipline<" << discipline_->name() << ">";
		}

		if (port_set_) {
			if (port_msb_ == 0) {
				cout << " port<scalar>";
			}
			else {
				cout << " port[" << *port_msb_ << ":" << *port_lsb_ << "]";
			}
		}
		if (net_set_) {
			if (net_msb_ == 0) {
				cout << " net<scalar>";
			}
			else {
				cout << " net[" << *net_msb_ << ":" << *net_lsb_ << "]";
			}
		}


		if (lidx_ || ridx_) {
			cout << "[";
			if (lidx_)
				out << *lidx_;
			if (ridx_)
				out << ":" << *ridx_;
			cout << "]";
		}

		cout << ";" << endl;
	}

	varsToType[basename()] = sectype_;

}


/**
 * Collect all expressions that  appears in dependent types.
 */
void Module::CollectDepExprs(ostream&out, TypeEnv & env) const {
	for (std::map<perm_string, SecType*>::iterator iter =
			(*env.varsToType).begin(); iter != (*env.varsToType).end(); iter++) {

		set<perm_string> exprs;

		for (const auto& pair : (*env.varsToType)) {
			exprs.insert(pair.first);
		}
		for (std::set<perm_string>::iterator expr = exprs.begin();
				expr != exprs.end(); expr++) {
			env.dep_exprs.insert(*expr);
		}
	}

	if (check_write)
		env.dep_exprs.insert(perm_string::literal("WriteLabel"));
}


std::string toStdString(const perm_string& ps) {
	return std::string(ps.str());
}


std::string setToString(const std::set<int>& s) {
	std::stringstream ss;


	auto it = s.begin();
	if (it != s.end()) {
		ss << *it;
		++it;
	}

	while (it != s.end()) {
		ss << "," << *it;
		++it;
	}

	return ss.str();
}


std::string nodeToString(const std::string& name, const std::set<int>& A, const std::set<int>& B, const std::set<int>& C) {
	std::stringstream ss;

	if (B.empty() && C.empty()) {
		ss << "new_basic_node(\"" << name << "\", ";
		ss << setToString(A);
		ss << ")";
	}
	else {
		ss << "new_capability_node(\"" << name << "\", ";
		ss << setToString(A);
		ss << ", ";
		ss << setToString(B);
		ss << ", ";
		ss << setToString(C);
		ss << ")";
	}

	return ss.str();
}


void printNodeToFile(std::ostream& out, const std::string& name, const std::string& mname, const std::set<int>& A, const std::set<int>& B, const std::set<int>& C) {
	if (B.empty() && C.empty()) {
		out  << name << " ";


		printSet(A, out);
		out << "\n";
	}
	else {

		out << name << " ";
		printSet(A, out);
		out << " ";
		printSet(B, out);
		out << " ";
		printSet(C, out);
		out << "\n";
	}
}

void printstart(std::ostream& out, const std::string& name)
{
	out << "N_" << name << ",";
}

void outputPorts(ostream & out, TypeEnv& env) {

    static std::set<std::string> seen_modules;

    if (env.module == nullptr) {
        return;
    }


	std::string current_module_name;
	if (env.current_instance_name.str()!= nullptr&&env.current_parent_name.str()!= nullptr) {
    	current_module_name = std::string(env.current_parent_name.str()) +"_"+std::string(env.current_instance_name.str());
	} else {
    	const char* m_name = env.module->mod_name().str();
        current_module_name = (m_name != nullptr) ? m_name : "TOP";
	}

    if (seen_modules.count(current_module_name)) {
        return;
    }


    seen_modules.insert(current_module_name);

		std::string instance_str = current_module_name;
		if (!instance_str.empty()) {
    instance_str[0] = std::toupper(instance_str[0]);
}
	for (const auto& pair : *env.varsToType) {
		const perm_string& name = pair.first;

		std::set<int>& A = (*env.varsToType)[name]->basic;
		std::set<int>& B = env.module->get_sec_label()->positive;

		std::set<int>& C = (*env.varsToType)[name]->negative;


        std::string ps = instance_str+ "_"+toStdString(name);
		std::string ps_m = instance_str + ".";
		if (!A.empty()) {
			printNodeToFile(out, ps, ps_m, A, B, C);
		}
		else {
			printNodeToFile(out, ps, ps_m,{1}, {}, {});
		}
	}


}


bool is_port_name_match(const Module* module_ptr, const perm_string& target_name) {
    static const Module* last_valid_module = nullptr;

    if (!target_name) {
        return false;
    }

    const Module* current_module = module_ptr;
    if (current_module == nullptr) {
        current_module = last_valid_module;
        if (current_module == nullptr) {
            return false;
        }
    } else {
        last_valid_module = current_module;
    }

    std::vector<perm_string> input_port_names;
	map<perm_string, PWire*> mwires;
	mwires = current_module->wires;
    for (const auto& pair : mwires) {
        PWire* port = pair.second;
        if (port != nullptr &&
            (port->get_port_type() == NetNet::PINPUT ||
             port->get_port_type() == NetNet::PINOUT)) {
            input_port_names.push_back(port->basename());
        }
    }

    for (const auto& input_port_name  : input_port_names) {
        if (input_port_name == target_name) {
            return true;
        }
    }

    return false;
}
template <typename Container>
bool has_equal_perm_string(const Container& rhs_vars, const Container& lhs_vars) {
    for (const perm_string& rhs : rhs_vars) {
        for (const perm_string& lhs : lhs_vars) {
            if (rhs == lhs) {
                return true;
            }
        }
    }
    return false;
}
/* Type-check a module. */
void Module::typecheck(ostream&out,ostream & PortsOut,ostream & AssignsOut,ostream & PortscheckOut,TypeEnv& env,
		map<perm_string, Module*>& modules, int current_index, std::vector<std::tuple<perm_string, perm_string, perm_string>>& child_modules) const {


  static std::set<std::string> seen_modules;

    if (env.module == nullptr) {
        return;
    }


	std::string current_module_name;
	if (env.current_instance_name.str()!= nullptr&&env.current_parent_name.str()!= nullptr) {
    	current_module_name = std::string(env.current_parent_name.str()) +"_"+std::string(env.current_instance_name.str());
	} else {
    	const char* m_name = env.module->mod_name().str();
        current_module_name = (m_name != nullptr) ? m_name : "TOP";
	}

    if (seen_modules.count(current_module_name)) {
        return;
    }


    seen_modules.insert(current_module_name);


	const perm_string modulename=this->mod_name();
	if (debug_typecheck) {
		cout << "Module::check " << mod_name() << endl;


		for (unsigned idx = 0; idx < ports.size(); idx += 1) {

			port_t* cur = ports[idx];

			if (cur == 0) {

				cout << "   unconnected" << endl;
				continue;
			}


			cout << "Port::check " << cur->name << "(" << *cur->expr[0];

			for (unsigned wdx = 1; wdx < cur->expr.size(); wdx += 1) {
				cout << ", " << *cur->expr[wdx];
			}

			cout << ")" << endl;
		}
	}

	typecheck_parameters_(out, env);
	typecheck_localparams_(out, env);

	typedef map<perm_string, LineInfo*>::const_iterator genvar_iter_t;
	for (genvar_iter_t cur = genvars.begin(); cur != genvars.end(); cur++) {

		(*env.varsToType)[(*cur).first] = ConstType::BOT;
	}

	typedef map<perm_string, PExpr*>::const_iterator specparm_iter_t;
	for (specparm_iter_t cur = specparams.begin(); cur != specparams.end();
			cur++) {
				throw "specparm";
	}

	typedef list<Module::named_expr_t>::const_iterator parm_hiter_t;
	for (parm_hiter_t cur = defparms.begin(); cur != defparms.end(); cur++) {
		throw "defparam";
	}

	typecheck_events_(out, env);


	typecheck_wires_(out, env);


	for (set<Equality*>::iterator invite = env.invariants->invariants.begin();
			invite != env.invariants->invariants.end();) {
		set<perm_string> vars, diff;
		(*invite)->left->collect_dep_expr(vars);
		(*invite)->right->collect_dep_expr(vars);
		set<Equality*>::iterator current = invite++;


		for (set<perm_string>::iterator varite = vars.begin();
				varite != vars.end(); varite++) {
			if (env.dep_exprs.find(*varite) == env.dep_exprs.end()) {
				env.invariants->invariants.erase(current);
				break;
			}
		}
	}


	typedef list<PGenerate*>::const_iterator genscheme_iter_t;
	for (genscheme_iter_t cur = generate_schemes.begin();
			cur != generate_schemes.end(); cur++) {

		(*cur)->typecheck(out,PortsOut,AssignsOut,PortscheckOut, env,  modules,current_index,this, child_modules);
	}


	typedef map<perm_string, PTask*>::const_iterator task_iter_t;
	for (task_iter_t cur = tasks.begin(); cur != tasks.end(); cur++) {
		out << "%PTask ignored" << endl;
	}


	typedef map<perm_string, PFunction*>::const_iterator func_iter_t;
	for (func_iter_t cur = funcs.begin(); cur != funcs.end(); cur++) {
	}


	outputPorts(PortsOut,env);
	map<perm_string, SecType*> varsToType_pool;
	map<perm_string, int> nodes_pool;
	for (list<PGate*>::const_iterator gate = gates_.begin();
			gate != gates_.end(); gate++) {

		PGAssign* pgassign = dynamic_cast<PGAssign*>(*gate);
		PGModule* pgmodule = dynamic_cast<PGModule*>(*gate);


	if (pgmodule != NULL ) {
		  perm_string old_inst = env.current_instance_name;
    perm_string old_parent = env.current_parent_name;


	std::string new_parent_path = (old_inst.str() != nullptr) ? toStdString(old_inst) : toStdString(this->mod_name());
    perm_string inst_name = pgmodule->get_name();

    if (inst_name.str() != nullptr) {
        env.current_instance_name = inst_name;
    } else {

        env.current_instance_name = perm_string();
    }


    if (old_parent.str() != nullptr) {
         new_parent_path = std::string(old_parent.str()) + "_" + new_parent_path;
    }

    env.current_parent_name = lex_strings.make(new_parent_path.c_str());
		pgmodule->typecheck(AssignsOut, PortscheckOut,env, modules, 0);
		env.current_instance_name = old_inst;
    env.current_parent_name = old_parent;
		perm_string module_type = pgmodule->get_type();
		map<perm_string, Module*>::const_iterator cur = modules.find(module_type);

		map<perm_string, PWire*> mwires;


        if (cur != modules.end()) {
			if(debug_typecheck) {cout<<"Instance name: "<<pgmodule->get_name().str()<<endl;
			}
			perm_string module_type = pgmodule->get_type();
			perm_string inst_name = pgmodule->get_name();
			perm_string parent_name = lex_strings.make(new_parent_path.c_str());
			child_modules.push_back(std::make_tuple(module_type, parent_name, inst_name));
		}


}else if (pgassign != NULL && !pgassign->is_checked()) {

            Predicate pred;
            pgassign->typecheck(AssignsOut, env, pred, current_index);
			pgassign->mark_checked();

    }

}


	for (list<PProcess*>::const_iterator behav = behaviors.begin();
			behav != behaviors.end(); behav++) {

		(*behav)->typecheck(AssignsOut, env,  current_index);

	}

	for (list<AProcess*>::const_iterator idx = analog_behaviors.begin();
			idx != analog_behaviors.end(); idx++) {
				cout<<"_AProcess_"<<endl;
		throw "AProcess";

	}

	for (list<PSpecPath*>::const_iterator spec = specify_paths.begin();
			spec != specify_paths.end(); spec++) {
				cout<<"_PSpecPath_"<<endl;
		throw "PSpecPath";
	}

	if(debug_typecheck)cout << endl << "true." << endl;


	varsToType_pool.clear();
	nodes_pool.clear();


}

/**
 * Type-check a process.Check process (always block)
 */
int PProcess::typecheck(ostream&out, TypeEnv& env,  int i) const {
	if (debug_typecheck) {
		cout << "PProcess:check " << " /* " << get_fileline() << " */" << endl;
	}
	stringstream ss;
	ss  << " @" << get_fileline();

	if (type_ != IVL_PR_INITIAL) {
		Predicate emptypred;
		if (auto* pfor = dynamic_cast<PForStatement*>(statement_))
		 {
			if(pfor->is_checked()) i=i;
			else i = statement_->typecheck(out, env,  emptypred, i,get_fileline());
			return i;
		}
		else
		i = statement_->typecheck(out, env,  emptypred, i,get_fileline());

	}
	return i;
}


/**
 * Do nothing.
 */
int AContrib::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i, std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________AContrib::check ";
		cout << *lval_ << " <+ " << *rval_ << endl;
	}
	throw "AContrib";
	return i;
}


std::string formatStringByMap(const std::string& pc_positive,
                              const std::map<perm_string, SecType*>& varsToType,
                              const std::string& instance_str) {
    std::string result = "";
    size_t pos = 0;
    size_t len = pc_positive.length();

    std::set<std::string> seen_keys;

    while (pos < len) {
        std::string longest_match_key = "";


        for (auto it = varsToType.begin(); it != varsToType.end(); ++it) {
            const std::string& key = it->first.str();
            size_t keyLen = key.length();

            if (keyLen > longest_match_key.length() && pos + keyLen <= len) {
                if (pc_positive.compare(pos, keyLen, key) == 0) {
                    longest_match_key = key;
                }
            }
        }


        if (!longest_match_key.empty()) {

            if (seen_keys.find(longest_match_key) == seen_keys.end()) {


                if (!result.empty()) {
                    result += ",";
                }

                if (!instance_str.empty()) {
                    result += instance_str + "_" + longest_match_key;
                } else {
                    result += longest_match_key;
                }


                seen_keys.insert(longest_match_key);
            }


            pos += longest_match_key.length();
        } else {

            pos++;
        }
    }
    return result;
}

/* Generate assignment constraints. */
int typecheck_assignment_constraint(ostream& out,
		Predicate pred, string note, string vardecl, TypeEnv* env,  PExpr* lexpr, PExpr* rexpr,int i, std::vector<std::string> rs_tokens, std::vector<std::string> ls_tokens) {


	out << vardecl;
	std::string current_module_name;

    if (env->current_instance_name.str()!= nullptr&&env->current_parent_name.str() != nullptr) {
    	current_module_name = std::string(env->current_parent_name.str()) +"_"+std::string(env->current_instance_name.str());
	} else {
    	current_module_name = env->module->mod_name();
	}


	std::string instance_str = current_module_name;
	std::string ls = toStdString(lexpr->get_name());
	std::string rs = toStdString(rexpr->get_name());
	if (!instance_str.empty()) {
    instance_str[0] = std::toupper(instance_str[0]);}


	std::string pc_positive;
	std::string pc_negative;


pc_positive = formatStringByMap(env->pcstring, (*env->varsToType),instance_str);
	std::set<std::string> basicVars;
	for (const auto& pair : (*env->varsToType)) {
		basicVars.insert(pair.first.str());
	}


	std::vector<std::string> rs_tokens_start = rs_tokens;

    std::set<std::string> unique_tokens(rs_tokens_start.begin(), rs_tokens_start.end());
    std::vector<std::string> rs_tokens_end(unique_tokens.begin(), unique_tokens.end());

    if(debug_typecheck){
			int index = 0;
    for (const auto& token : rs_tokens_end) {
    std::cout << "Token_end[" << index++ << "]: " << token << std::endl;
    }
    }


	Constraint* c = new Constraint(env->invariants, &pred,  note, ls,rs, basicVars, rs_tokens_end,ls_tokens,env->varsToType,0, pc_positive,pc_negative,instance_str,instance_str);
	if(debug_typecheck)
	{cout<<*c<<endl;}

	out << *c;

	i++;
	delete c;
	if (check_write) {

		out << vardecl;
		c = new Constraint(env->invariants, &pred,  note,ls,rs,basicVars, rs_tokens_end,ls_tokens,env->varsToType,0, pc_positive,pc_negative,instance_str,instance_str);
		i++;
		out << *c;
		delete c;


	}
	return i;
}
bool is_only_whitespace(const std::string& str) {
	return std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c) != 0; });
}


/**
 * Type-check an assignment (either noblocking or blocking).
 */
int typecheck_assignment(ostream& out, PExpr* lhs, PExpr* rhs, TypeEnv* env,
		Predicate &precond, Predicate postcond, unsigned int lineno, string note,int i, std::string r) {

	PETernary* ternary = dynamic_cast<PETernary*>(rhs);

	if (ternary == NULL) {


		std::vector<std::string> rs_tokens;
		std::vector<std::string> ls_tokens;

		rs_tokens=rhs->typecheckstring((*env->varsToType));
		ls_tokens=lhs->typecheckstring((*env->varsToType));

		if (!lhs->typecheck(out, (*env->varsToType))->hasExpr(lhs->get_name())) {
			typecheck_assignment_constraint(out,   precond, note,
					"", env,  lhs,rhs,i, rs_tokens,ls_tokens);
		} else {

			stringstream ss, vardecl;
			ss << lhs->get_name() << lineno;


			string tmp_str = ss.str();
			perm_string newname = perm_string::literal(tmp_str.c_str());
			vardecl << "(declare-fun " << newname << " () Int)" << endl;
			map<perm_string, PWire*>::const_iterator cur =
					env->module->wires.find(lhs->get_name());
			if (cur != env->module->wires.end()) {
				PWire* def = (*cur).second;
				vardecl << "(assert (<= 0  " << newname << "))" << endl;
				vardecl << "(assert (<= " << newname << " "
						<< (1 << (def->getRange() + 1)) - 1 << "))" << endl;
			}


			set<perm_string>::iterator aiter = env->aliveVars.find(
					lhs->get_name());
			if (aiter != env->aliveVars.end()) {

				typecheck_assignment_constraint(out,  precond,
						"no-sensitive-upgrade check " + note, "", env ,  lhs,rhs,i, rs_tokens,ls_tokens);
			}

			PENumber* number = dynamic_cast<PENumber*>(rhs);
			if (number != NULL
					|| env->dep_exprs.find(rhs->get_name())
							!= env->dep_exprs.end()) {
				precond.hypotheses.insert(
						new Hypothesis(new PEIdent(newname), rhs));
			}
			typecheck_assignment_constraint(out,
					 precond,
					note, vardecl.str(), env ,  lhs,rhs,i, rs_tokens,ls_tokens);

		}
	} else {
		ternary->translate(lhs)->typecheck(out, *env,  precond,i,r);
	}
	return i;
}

/**
 * Type-check a blocking assignment.
 */
int PGAssign::typecheck(ostream&out, TypeEnv& env,  Predicate pred,int i) const {
	if (debug_typecheck) {
		cout << "assign " << *pin(0) << " = " << *pin(1) << ";" << endl;
	}

	stringstream ss;
	ss << "assign " << *pin(0) << " = " << *pin(1) << " @" << get_fileline();

	Predicate post;


	i=typecheck_assignment(out, pin(0), pin(1), &env,  pred, post, get_lineno(),
			ss.str(),i,"1");


	return i;
}


/**
 * Type-check a module instantiation.
 */
int PGModule::typecheck(ostream&out, ostream&PortscheckOut,TypeEnv& env,
		map<perm_string, Module*>& modules,int i) {

	std::set<std::string> basicVars;
    static std::set<std::string> seen_modules;


	for (const auto& pair : (*env.varsToType)) {
		basicVars.insert(pair.first.str());
	}

	this->pin_count();
	this->get_pin_count();


std::string current_module_name;

const char* parent_name = env.current_parent_name.str();
const char* instance_name = env.current_instance_name.str();
if (parent_name != nullptr && instance_name != nullptr) {
    current_module_name = std::string(parent_name) + "_" + std::string(instance_name);
} else {
    const char* m_name = env.module->mod_name().str();
    current_module_name = (m_name != nullptr) ? m_name : "TOP";
}


    seen_modules.insert(current_module_name);
	std::string modinstance_str =(env.current_parent_name.str() != nullptr) ? std::string(env.current_parent_name.str()) : "";


	if(debug_typecheck) cout << "Found Module Name: " << modinstance_str << endl;
	if (!modinstance_str.empty()) {
    modinstance_str[0] = std::toupper(modinstance_str[0]);}


	std::string instance_str = current_module_name;
	if (!instance_str.empty()) {
    instance_str[0] = std::toupper(instance_str[0]);}


	map<perm_string, Module*>::const_iterator cur = modules.find(get_type());

	map<perm_string, PWire*> mwires;

	if (cur != modules.end()) {
		mwires = ((*cur).second)->wires;


		unsigned pincount = get_pin_count();
		if (pincount != 0) {


			for (unsigned idx = 0; idx < pincount; idx += 1) {
				map<perm_string, PWire*>::const_iterator ite = mwires.find(
						get_pin_name(idx));
				if (ite != mwires.end()) {
					PWire* port = (*ite).second;
					PExpr* actual = get_param(idx);
					if (port->get_port_type() == NetNet::PINPUT
							|| port->get_port_type() == NetNet::PINOUT) {

						if (actual) {

						} else {
    							PortscheckOut << "\033[34m"
    							<< "Error: input port '" << get_pin_name(idx) << "' unconnected"<< " in module "
									<< get_type() << " @" << get_fileline()
									<< "\033[0m" << endl;

						}
					} else {

					}

					if (port->get_port_type() == NetNet::POUTPUT
							|| port->get_port_type() == NetNet::PINOUT) {

						if (actual) {


						} else {
    							PortscheckOut << "\033[34m"
    							<< "Waring: output port '" << get_pin_name(idx) << "' unconnected"<< " in module "
									<< get_type() << " @" << get_fileline()
									<< "\033[0m" << endl;

						}


					} else {

						if (actual) {

						} else {
    							PortscheckOut << "\033[34m"
    							<< "Error: input port '" << get_pin_name(idx) << "' unconnected"<< " in module "
									<< get_type() << " @" << get_fileline()
									<< "\033[0m" << endl;

						}
					}
				}
			}
			for (unsigned idx = 0; idx < pincount; idx += 1) {
				map<perm_string, PWire*>::const_iterator ite = mwires.find(
						get_pin_name(idx));
				if (ite != mwires.end()) {
					PExpr* param = get_param(idx);

					if (param != NULL) {
						PWire* port = (*ite).second;
						NetNet::PortType porttype = port->get_port_type();


						set<perm_string> vars;

						for (set<perm_string>::iterator varsite = vars.begin();
								varsite != vars.end(); varsite++) {
							if (env.dep_exprs.find(*varsite)
									== env.dep_exprs.end()) {
								out << "(declare-fun " << (*varsite)
										<< " () Int)" << endl;
								map<perm_string, PWire*>::const_iterator decl =
										mwires.find(*varsite);

								if (decl != mwires.end()) {
									PWire* def = (*decl).second;
									out << "(assert (<= 0  " << (*varsite)
											<< "))" << endl;
									out << "(assert (<= " << (*varsite) << " "
											<< (1 << (def->getRange() + 1)) - 1
											<< "))" << endl;
								}
							}
						}
						std::vector<std::string> rs_tokens,ls_tokens;
						std::string note = toStdString(get_pin_name(idx))+" in module "+ toStdString(get_type())+" @"+ get_fileline();
						std::string pcstring;


						pcstring = formatStringByMap(env.pcstring, (*env.varsToType),instance_str);
						if ((porttype == NetNet::PINPUT
								|| porttype == NetNet::PINOUT)){


							rs_tokens=param->typecheckstring((*env.varsToType));

							Predicate pred;
							std::string ls = toStdString(port->basename());
							std::string rs = toStdString(param->get_name());
							ls_tokens.push_back(toStdString(port->basename()));
							Constraint* c = new Constraint(
									env.invariants, &pred,  note, ls, rs,basicVars, rs_tokens,ls_tokens,env.varsToType,1,pcstring,"0",modinstance_str,instance_str);
							out << *c;
							delete c;
						} else if ((porttype == NetNet::POUTPUT
								|| porttype == NetNet::PINOUT)) {


							rs_tokens.push_back(std::string(port->basename().str()));
							Predicate pred;
							std::string ls = toStdString(param->get_name());
							std::string rs = toStdString(port->basename());
							ls_tokens=param->typecheckstring((*env.varsToType));
							Constraint* c = new Constraint(
									env.invariants, &pred,  note,ls, rs,basicVars, rs_tokens,ls_tokens,env.varsToType,2,pcstring, "0",modinstance_str,instance_str);

							out << *c;
							delete c;
						}
					} else {

					}
				} else {

					cout << "PWire " << get_pin_name(idx) << " is not found"<<" in module "
						<< get_type() << " @" << get_fileline()
							<< endl;
				}
			}
		}
	}
	return i;
}

/* Type-check an assignment statement. */
int PAssign::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "PAssign::check " << *lval() << " = ";
		if (delay_)
			cout << "#" << *delay_ << " ";
		if (count_)
			cout << "repeat(" << *count_ << ") ";
		if (event_)
			cout << *event_ << " ";
		cout << *rval() << ";" << endl;
	}

	stringstream ss;
	ss << *lval() << " = " << *rval() << " @" << get_fileline();
	Predicate precond = pred;
	absintp(pred, env);


	i=typecheck_assignment(out, lval_, rval_, &env,  precond, pred, get_lineno(),
			ss.str(),i,r);
	return i;
}

/**
 * Type-check a nonblocking assignment. Non-blocking
 */
int PAssignNB::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "PAssignNB::check " << *lval() << " <= ";
		if (delay_)
			cout << "#" << *delay_ << " ";
		if (count_)
			cout << "repeat(" << *count_ << ") ";
		if (event_)
			cout << *event_ << " ";
		cout << *rval() << ";" << endl;
	}

	stringstream ss;
	ss << *lval() << " <= " << *rval() << " @" << get_fileline();
	Predicate precond = pred;
	absintp(pred, env);

	std::vector<std::string> rs_tokens;
	rs_tokens=rval_->typecheckstring((*env.varsToType));
	i=typecheck_assignment(out, lval_, rval_, &env,  precond, pred, get_lineno(),
			ss.str(),i,r);


	return i;
}

/**
 * Type-check a block of code.
 */
int PBlock::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "PBlock::check " << "begin";
		if (pscope_name() != 0)
			cout << " : " << pscope_name();
		cout << endl;
	}

	if (pscope_name() != 0) {
		typecheck_parameters_(out, env);

		typecheck_localparams_(out, env);

		typecheck_events_(out, env);


		typecheck_wires_(out, env);
	}

	set<Hypothesis*> before = pred.hypotheses;
	for (unsigned idx = 0; idx < list_.count(); idx += 1) {
		if (list_[idx])
		{
			if (auto* pfor = dynamic_cast<PForStatement*>(list_[idx]))
		 {if(pfor->is_checked()) i=i;
			else i=list_[idx]->typecheck(out, env,  pred,i,r);}
		else
			i=list_[idx]->typecheck(out, env,  pred,i,r);}
	}
	return i;
}


/**
 * Do nothing.
 */
int PCallTask::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PCallTask::check " << path_;

		if (parms_.count() > 0) {
			cout << "(";
			if (parms_[0])
				cout << *parms_[0];

			for (unsigned idx = 1; idx < parms_.count(); idx += 1) {
				cout << ", ";
				if (parms_[idx])
					cout << *parms_[idx];
			}
			cout << ")";
		}
		cout << endl;
	}
	throw "PCallTask";


	return i;
}


/**
 * Type-check a case statement.
 */
int PCase::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "PCase::check ";

		cout << " (" << *expr_ << ") " << endl;

		for (unsigned idx = 0; idx < items_->count(); idx += 1) {
			PCase::Item*cur = (*items_)[idx];

			if (cur->expr.count() != 0) {
				cout << "case item " << *cur->expr[0];

				for (unsigned e = 1; e < cur->expr.count(); e += 1)
					cout << ", " << *cur->expr[e];

				cout << ":";
			}
		}
	}
    if (!expr_ || !items_) {
        cerr << "PCase::typecheck: null expr_ or items_" << endl;
        return i;
    }
	if(debug_typecheck){

    void** vtable = *(void***)expr_;
    cerr << "expr_ vtable: " << vtable << endl;
    if (!vtable || vtable == (void**)0x1 || vtable == (void**)0x0) {
        cerr << "ERROR: expr_ has invalid vtable!" << endl;
        return i;
    }
	cerr << "items_->count(): " << items_->count() << endl;}
	for (unsigned idx = 0; idx < items_->count(); idx += 1) {
		PCase::Item*cur = (*items_)[idx];
		if (!cur) continue;

		SecType* oldpc = env.pc;
		string oldpcstring=env.pcstring;
		bool need_hypo = env.dep_exprs.find(expr_->get_name())
				!= env.dep_exprs.end();

		env.pc = new JoinType(expr_->typecheck(out, (*env.varsToType)), oldpc);
		env.pc = env.pc->simplify();
		env.pcstring=expr_->toString()+oldpcstring;

		Predicate oldh = pred;

if (need_hypo) {
    if (cur->expr.count() != 0) {
        auto& hs = pred.hypotheses;
        for (auto it = hs.begin(); it != hs.end(); ) {
            Hypothesis* hyp = *it;
            PExpr* left = hyp->getL();

            if (left && left == expr_) {
                delete hyp;
                it = hs.erase(it);
            } else {
                ++it;
            }
        }

        Hypothesis* newHyp = new Hypothesis(expr_, cur->expr[0]);
        pred.hypotheses.insert(newHyp);
    }
}
		if (cur->stat) {
			if (auto* pfor = dynamic_cast<PForStatement*>(cur->stat))
		 {if(pfor->is_checked()) i=i;
			else i=cur->stat->typecheck(out, env,
					pred ,i,r);}
		else
			i=cur->stat->typecheck(out, env,
					pred ,i,r);
		}

		pred = oldh;
		env.pc = oldpc;
		env.pcstring = oldpcstring;
	}
	return i;

}


/**
 * Type-check a branch.
 */
int PCondit::typecheck(ostream&out, TypeEnv& env, Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "PCondit::check " << "if (" << *expr_ << ")" << endl;
		cout << pred << endl;
	}

	SecType* oldpc = env.pc;
	string oldpcstring=env.pcstring;
	set<Hypothesis*> beforeif = pred.hypotheses;
	set<perm_string> oldalive = env.aliveVars;


	SecType* etype = expr_->typecheck(out, (*env.varsToType));
	env.pc = new JoinType(etype, oldpc);
	env.pc = env.pc->simplify();
	env.pcstring=expr_->toString();


	set<PExpr*, ExprComparator> modified;

	mustmodify(modified, env.dep_exprs);

	for (set<perm_string>::iterator depite = env.dep_exprs.begin();
		depite != env.dep_exprs.end(); depite++) {
		bool add = true;
		for (set<PExpr*, ExprComparator>::iterator exprite = modified.begin();
			exprite != modified.end(); exprite++) {
			if ((*exprite)->get_name() == *depite) {
				add = false;
				break;
			}
		}
		if (add)
			env.aliveVars.insert(*depite);
	}


	if (if_) {

		absintp(pred, env, true);
        if (auto* pfor = dynamic_cast<PForStatement*>(if_))
		 {if(pfor->is_checked()) i=i;
			else i=if_->typecheck(out, env,pred,i,r);}
		else
		i=if_->typecheck(out, env,pred,i,r);
	}
	Predicate afterif = pred;
	pred.hypotheses = beforeif;

	int i_after_if = i;
	if (else_) {
		absintp(pred, env, false);
		if (auto* pfor = dynamic_cast<PForStatement*>(else_))
		 {if(pfor->is_checked()) i=i;
			else i=else_->typecheck(out, env, pred, i_after_if,r);}
		else
		i=else_->typecheck(out, env, pred, i_after_if,r);
	}
	Predicate afterelse = pred;


	env.pc = oldpc;
	env.pcstring=oldpcstring;
	env.aliveVars = oldalive;
	pred.hypotheses.clear();
	merge(afterif, afterelse, pred);
	return i;

}


/**
 * Do nothing.
 */
int PCAssign::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PCAssign::check " << "assign " << *lval_ << " = " << *expr_
				<< endl;
	}
	throw "PCAssign";
	return i;
}

/**
 * Do nothing.
 */
int PDeassign::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PDeassign::check " << *lval_ << "; " << endl;
	}
	throw "PDeassign";
	return i;
}

/**
 * Do nothing.
 */
int PDelayStatement::typecheck(ostream&out, TypeEnv& env,
		Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PDelayStatement::check " << "#" << *delay_;
		if (statement_) {
			cout << endl;
		} else {
			cout << " /* noop */;" << endl;
		}
	}
	throw "PDelay";
	return i;
}

/**
 * Do nothing.
 */
int PDisable::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PDisable::check " << scope_ << ";" << endl;
	}
	throw "PDisable";
	return i;
}

/**
 * Type-check an event.
 */
int PEventStatement::typecheck(ostream&out, TypeEnv& env,
		Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		if (expr_.count() == 0) {
			cout << "PEventStatement::check " << "@* ";
		} else {
			cout << "PEventStatement::check" << "@(" << *(expr_[0]);
			if (expr_.count() > 1)
				for (unsigned idx = 1; idx < expr_.count(); idx += 1)
					cout << " or " << *(expr_[idx]);
			cout << ")";
		}

		if (statement_) {
			cout << endl;
		} else {
			cout << " ;" << endl;
		}
	}

	SecType* oldpc = env.pc;
	string oldpcstring=env.pcstring;


	if (expr_.count() != 0) {

		if (expr_[0] != nullptr && expr_[0]->expr() != nullptr && env.varsToType != nullptr) {
			env.pc = new JoinType(env.pc,
					expr_[0]->expr()->typecheck(out, (*env.varsToType)));
			env.pcstring = env.pcstring + expr_[0]->expr()->toString();
		}

		for (unsigned idx = 1; idx < expr_.count(); idx += 1) {

			if (expr_[idx] != nullptr && expr_[idx]->expr() != nullptr && env.varsToType != nullptr) {
				env.pc = new JoinType(env.pc,
					expr_[idx]->expr()->typecheck(out, (*env.varsToType)));
				env.pcstring = env.pcstring + expr_[idx]->expr()->toString();
			}
		}
	}


	if (statement_ != nullptr) {
		if (auto* pfor = dynamic_cast<PForStatement*>(statement_)) {
			if (pfor->is_checked()) {
				i = i;
			} else {
				i = statement_->typecheck(out, env, pred, i, r);
			}
		} else {
			i = statement_->typecheck(out, env, pred, i, r);
		}
	}

	env.pc = oldpc;
	env.pcstring=oldpcstring;

	return i;
}


int PForce::typecheck(ostream&out, TypeEnv& env,  Predicate& pred,int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PForce::check " << *lval_ << " = " << *expr_ << ";" << endl;
	}
	throw "PForce";
	return i;
}

int PForever::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PForever::check " << endl;
	}
	throw "PForever";
	return i;
}


/**
 * Do nothing.
 */
int PForStatement::typecheck(ostream&out, TypeEnv& env,
		Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "PForStatement::check " << "for (" << *name1_ << " = "
				<< *expr1_ << "; " << *cond_ << "; " << *name2_ << " = "
				<< *expr2_ << ")" << endl;
	}

	const_cast<PForStatement*>(this)->mark_checked();

	return i;
}

/**
 * Type-check a generate statement.
 * Use a copy of env to avoid duplicated definitions.
 */


int PGenerate::typecheck(ostream&out,ostream & PortsOut,ostream & AssignsOut,ostream & PortscheckOut,TypeEnv &env,
		map<perm_string, Module*>& modules, int current_index,const Module* module_ptr, std::vector<std::tuple<perm_string, perm_string, perm_string>>& child_modules) {

	if (debug_typecheck) {
		dump(cout, 0);
	}
	 static std::set<std::string> seen_modules;

    if (env.module == nullptr) {
        return 0;
    }


	std::string current_module_name;
	if (env.current_instance_name.str()!= nullptr&&env.current_parent_name.str()!= nullptr) {
    	current_module_name = std::string(env.current_parent_name.str()) +"_"+std::string(env.current_instance_name.str());
	} else {
    	const char* m_name = env.module->mod_name().str();
        current_module_name = (m_name != nullptr) ? m_name : "TOP";
	}

    if (seen_modules.count(current_module_name)) {
        return 0;
    }


    seen_modules.insert(current_module_name);


	typecheck_parameters_(AssignsOut, env);
	typecheck_localparams_(AssignsOut, env);
	if (defparms.size() > 0) {
		cout << "PGenerate::defparms are ignored" << endl;
	}
	typecheck_events_(AssignsOut, env);

	typecheck_wires_(AssignsOut, env);

	map<perm_string, SecType*> varsToType_pool;
	map<perm_string, int> nodes_pool;
	for (list<PGate*>::const_iterator gate = gates.begin(); gate != gates.end();
			gate++) {

		PGAssign* pgassign = dynamic_cast<PGAssign*>(*gate);
		PGModule* pgmodule = dynamic_cast<PGModule*>(*gate);


		if (pgmodule != NULL) {
			perm_string old_inst = env.current_instance_name;
    perm_string old_parent = env.current_parent_name;


	std::string new_parent_path = (old_inst.str() != nullptr) ? toStdString(old_inst) : toStdString(module_ptr->mod_name());
    perm_string inst_name = pgmodule->get_name();

    if (inst_name.str() != nullptr) {
        env.current_instance_name = inst_name;
    } else {

        env.current_instance_name = perm_string();
    }


    if (old_parent.str() != nullptr) {
         new_parent_path = std::string(old_parent.str()) + "_" + new_parent_path;
    }

    env.current_parent_name = lex_strings.make(new_parent_path.c_str());
		pgmodule->typecheck(AssignsOut, PortscheckOut,env, modules, 0);
		env.current_instance_name = old_inst;
    env.current_parent_name = old_parent;


			map<perm_string, Module*>::const_iterator cur = modules.find(pgmodule->get_type());

		    if (cur != modules.end()) {
				Module* module_def = cur->second;


				if(debug_typecheck){cout << "Instance name: " <<pgmodule->get_name()<< endl;
				}
				perm_string module_type = pgmodule->get_type();
				perm_string inst_name = pgmodule->get_name();
				perm_string parent_name = lex_strings.make(new_parent_path.c_str());
				child_modules.push_back(std::make_tuple(module_type, parent_name, inst_name));
			}

		} else if (pgassign != NULL) {
			Predicate pred;
			pgassign->typecheck(AssignsOut, env,  pred, current_index);

		}

	}
	for (list<PProcess*>::const_iterator idx = behaviors.begin();
			idx != behaviors.end(); idx++) {

		(*idx)->typecheck(AssignsOut, env,  current_index);

	}

	for (list<AProcess*>::const_iterator idx = analog_behaviors.begin();
			idx != analog_behaviors.end(); idx++) {
				cout<<"errorAProcess" << endl;
		throw "AProcess";

	}

	typedef map<perm_string, LineInfo*>::const_iterator genvar_iter_t;
	for (genvar_iter_t cur = genvars.begin(); cur != genvars.end(); cur++) {


		throw "genvars";
	}

	for (list<PGenerate*>::const_iterator idx = generate_schemes.begin();
			idx != generate_schemes.end(); idx++) {
		(*idx)->typecheck(out,PortsOut,AssignsOut,PortscheckOut, env,  modules,current_index,module_ptr, child_modules);
	}


	return current_index;

}

/**
 * Do nothing.
 */
int PNoop::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "check PNoop_______________________________________________________________ ______________________________________________________________________________________________________________________________" << endl;
	}
	throw "PNoop";
	return i;
}

/**
 * Do nothing.
 */
int PRelease::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PRelease::check " << *lval_ << ";" << endl;
	}
	throw "PRelease";
	return i;
}

/**
 * Do nothing.
 */
int PRepeat::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PRepeat::check (" << *expr_ << ")" << endl;
	}
	throw "PRepeat";
	return i;
}

/**
 * Do nothing.
 */
int PTrigger::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PTrigger::check" << "-> " << event_ << ";" << endl;
	}
	throw "PTrigger";
	return i;
}

/**
 * Do nothing.
 */
int PWhile::typecheck(ostream&out, TypeEnv& env,  Predicate& pred, int i,std::string r) const {
	if (debug_typecheck) {
		cout << "_____________________________________________________________________________________________________________________________________________________________________________________________PWhile::check (" << *cond_ << ")" << endl;
	}
	throw "PWhile";
	return i;
}


struct root_elem {
	Module *mod;
	NetScope *scope;
};




void typecheck(map<perm_string, Module*> modules,
	list<perm_string> roots) {

	if (debug_typecheck) {
		cerr << "===== DEBUG: roots list =====" << endl;
		cerr << "roots size: " << roots.size() << endl;

		int count = 0;
		for (perm_string root : roots) {
			cerr << "roots[" << count++ << "]: \"" << root << "\"" << endl;
		}

		cerr << "===== DEBUG: available modules =====" << endl;
		cerr << "modules size: " << modules.size() << endl;

		count = 0;
		for (auto& mod : modules) {
			if (count < 20) {
				cerr << "module[" << count << "]: \"" << mod.first << "\"" << endl;
			}
			count++;
		}
		if (count > 20) {
			cerr << "... and " << (count - 20) << " more modules" << endl;
		}
		cerr << "===== END DEBUG =====" << endl;
	}

	ofstream nodefile, stepfile, portfile;
	stringstream TypeOut;
	string output_stem;
	Module* last_rmod = nullptr;

	g_current_modules = &modules;

	if (!roots.empty() && roots.front().str() != nullptr) {
		output_stem = roots.front().str();
	} else {
		last_rmod = modules.begin()->second;
	}

	if (output_stem.empty() && last_rmod != nullptr) {
		string full_path = string(last_rmod->file_name().str());

		size_t last_slash = full_path.find_last_of('/');
		if (last_slash != string::npos) {
			output_stem = full_path.substr(last_slash + 1);
		} else {
			output_stem = full_path;
		}

		size_t pos = output_stem.find_first_of('.');
		if (pos != string::npos) {
			output_stem = output_stem.substr(0, pos);
		}

	}

	std::transform(output_stem.begin(), output_stem.end(), output_stem.begin(),
		[](unsigned char c) { return std::tolower(c); });

	nodefile.open((output_stem + "_nodes.txt").c_str(), std::ios::out | std::ios::trunc);
	nodefile.close();
	nodefile.open((output_stem + "_nodes.txt").c_str(), std::ios::app);

	stepfile.open((output_stem + "_steps.txt").c_str(), std::ios::out | std::ios::trunc);
	stepfile.close();
	stepfile.open((output_stem + "_steps.txt").c_str(), std::ios::app);

	portfile.open((output_stem + "_portcheck.txt").c_str(), std::ios::out | std::ios::trunc);
	portfile.close();
	portfile.open((output_stem + "_portcheck.txt").c_str(), std::ios::app);

	map<perm_string, SecType*> varsToType_pool;
	map<perm_string, int> nodes_pool;

	typedef std::tuple<perm_string, perm_string, perm_string> ModuleTask;
	std::vector<ModuleTask> current_level;
	std::vector<ModuleTask> next_level;

	for (perm_string root : roots) {
		if (modules.find(root) != modules.end()) {
			current_level.push_back(std::make_tuple(root, perm_string(), perm_string()));
		}
	}

	while (!current_level.empty()) {
		next_level.clear();

		for (const ModuleTask& task : current_level) {
			perm_string module_type = std::get<0>(task);
			perm_string parent_name = std::get<1>(task);
			perm_string inst_name = std::get<2>(task);

			auto it = modules.find(module_type);
			if (it == modules.end()) continue;

			Module* rmod = it->second;

			varsToType_pool.clear();
			nodes_pool.clear();
			/* SecMaxType/SecDownType are legacy placeholders here. */
			TypeEnv env(varsToType_pool, ConstType::BOT, SecMaxType::TOP, SecDownType::BOT, rmod);
			env.current_parent_name = parent_name;
			if (inst_name.str() != nullptr) {
				env.current_instance_name = inst_name;
			} else {
				env.current_instance_name = perm_string();
			}

			std::vector<ModuleTask> child_modules;

			try {
				if(debug_typecheck) {
					cout << "Processing module: " << rmod->file_name().str() << endl;
				}
				int current_index = 1;
				rmod->typecheck(TypeOut, nodefile, stepfile, portfile, env, modules,
							   current_index, child_modules);
			}
			catch (char const* str) {
				cerr << "Unimplemented " << str << endl;
			}

			for (const ModuleTask& child : child_modules) {
				next_level.push_back(child);
			}

			varsToType_pool.clear();
			nodes_pool.clear();
		}

		varsToType_pool.clear();
		nodes_pool.clear();
		current_level.swap(next_level);
	}

	if (debug_typecheck) {
        cout << "Type checking complete." << endl;
    }

	nodefile.close();


	stepfile.close();
	portfile.close();


		}

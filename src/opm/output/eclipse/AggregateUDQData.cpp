/*
  Copyright 2018 Statoil ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/output/eclipse/AggregateUDQData.hpp>
#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>
#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQDefine.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <iostream>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateGroupData
// ---------------------------------------------------------------------

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;
namespace {

    // maximum number of groups
    std::size_t ngmaxz(const std::vector<int>& inteHead)
    {
        return inteHead[20];
    }

    // maximum number of wells
    std::size_t nwmaxz(const std::vector<int>& inteHead)
    {
        return inteHead[163];
    }


    // function to return true if token is a function
    bool tokenTypeFunc(const Opm::UDQTokenType& token) {
        bool type = false;
        std::vector <Opm::UDQTokenType> type_1 = {
            Opm::UDQTokenType::elemental_func_randn,
            Opm::UDQTokenType::elemental_func_randu,
            Opm::UDQTokenType::elemental_func_rrandn,
            Opm::UDQTokenType::elemental_func_rrandu,
            Opm::UDQTokenType::elemental_func_abs,
            Opm::UDQTokenType::elemental_func_def,
            Opm::UDQTokenType::elemental_func_exp,
            Opm::UDQTokenType::elemental_func_idv,
            Opm::UDQTokenType::elemental_func_ln,
            Opm::UDQTokenType::elemental_func_log,
            Opm::UDQTokenType::elemental_func_nint,
            Opm::UDQTokenType::elemental_func_sorta,
            Opm::UDQTokenType::elemental_func_sortd,
            Opm::UDQTokenType::elemental_func_undef,
            //
            Opm::UDQTokenType::scalar_func_sum,
            Opm::UDQTokenType::scalar_func_avea,
            Opm::UDQTokenType::scalar_func_aveg,
            Opm::UDQTokenType::scalar_func_aveh,
            Opm::UDQTokenType::scalar_func_max,
            Opm::UDQTokenType::scalar_func_min,
            Opm::UDQTokenType::scalar_func_norm1,
            Opm::UDQTokenType::scalar_func_norm2,
            Opm::UDQTokenType::scalar_func_normi,
            Opm::UDQTokenType::scalar_func_prod,
            Opm::UDQTokenType::table_lookup
        };
        for (const auto& tok_type : type_1) {
            if (token == tok_type) {
                type = true;
                break;
            }
        }
        return type;
    }

// function to return true if token is a binary operator: type compare
    bool tokenTypeBinaryCmpOp(const Opm::UDQTokenType& token) {
        bool type = false;
        std::vector <Opm::UDQTokenType> type_1 = {
            Opm::UDQTokenType::binary_cmp_eq,
            Opm::UDQTokenType::binary_cmp_ne,
            Opm::UDQTokenType::binary_cmp_le,
            Opm::UDQTokenType::binary_cmp_ge,
            Opm::UDQTokenType::binary_cmp_lt,
            Opm::UDQTokenType::binary_cmp_gt

        };
        for (const auto& tok_type : type_1) {
            if (token == tok_type) {
                type = true;
                break;
            }
        }
        return type;
    }

    // function to return true if token is a binary operator: type power (exponentiation)
    bool tokenTypeBinaryPowOp(const Opm::UDQTokenType& token) {
        return (token == Opm::UDQTokenType::binary_op_pow) ? true: false;
    }

    // function to return true if token is a binary operator: type multiply or divide
    bool tokenTypeBinaryMulDivOp(const Opm::UDQTokenType& token) {
        bool type = false;
        std::vector <Opm::UDQTokenType> type_1 = {
            Opm::UDQTokenType::binary_op_div,
            Opm::UDQTokenType::binary_op_mul
        };
        for (const auto& tok_type : type_1) {
            if (token == tok_type) {
                type = true;
                break;
            }
        }
        return type;
    }

    // function to return true if token is a binary operator: type add or subtract
    bool tokenTypeBinaryAddSubOp(const Opm::UDQTokenType& token) {
        bool type = false;
        std::vector <Opm::UDQTokenType> type_1 = {
            Opm::UDQTokenType::binary_op_add,
            Opm::UDQTokenType::binary_op_sub
        };
        for (const auto& tok_type : type_1) {
            if (token == tok_type) {
                type = true;
                break;
            }
        }
        return type;
    }

    // function to return true if token is a binary operator: type
    bool tokenTypeBinaryUnionOp(const Opm::UDQTokenType& token) {
        bool type = false;
        std::vector <Opm::UDQTokenType> type_1 = {
            Opm::UDQTokenType::binary_op_uadd,
            Opm::UDQTokenType::binary_op_umul,
            Opm::UDQTokenType::binary_op_umin,
            Opm::UDQTokenType::binary_op_umax
        };
        for (const auto& tok_type : type_1) {
            if (token == tok_type) {
                type = true;
                break;
            }
        }
        return type;
    }

    // function to return true if token is a binary operator: anytype
    bool tokenTypeBinaryOp(const Opm::UDQTokenType& token) {
        bool type = false;
        std::vector <Opm::UDQTokenType> type_1 = {
            Opm::UDQTokenType::binary_op_add,
            Opm::UDQTokenType::binary_op_sub,
            Opm::UDQTokenType::binary_op_div,
            Opm::UDQTokenType::binary_op_mul,
            Opm::UDQTokenType::binary_op_pow,
            Opm::UDQTokenType::binary_op_uadd,
            Opm::UDQTokenType::binary_op_umul,
            Opm::UDQTokenType::binary_op_umin,
            Opm::UDQTokenType::binary_op_umax,
            Opm::UDQTokenType::binary_cmp_eq,
            Opm::UDQTokenType::binary_cmp_ne,
            Opm::UDQTokenType::binary_cmp_le,
            Opm::UDQTokenType::binary_cmp_ge,
            Opm::UDQTokenType::binary_cmp_lt,
            Opm::UDQTokenType::binary_cmp_gt

        };
        for (const auto& tok_type : type_1) {
            if (token == tok_type) {
                type = true;
                break;
            }
        }
        return type;
    }

    // function to return the number of sets of () outside any function / operator or variable
    int noOutsideOpencloseParen(const std::vector<Opm::UDQToken>& tokens) {
        bool last_paren = false;
        int noOpenPar = 0;
        int noOCPPairs = 0;
        auto modTokens = tokens;
        bool lastTokCP = true;
        std::cout << "noOutsideOpencloseParen" << std::endl;
        while (lastTokCP) {
            bool firstTokenOpenParen = (modTokens[0].type() == Opm::UDQTokenType::open_paren) ? true : false;
            if (firstTokenOpenParen) {
                noOpenPar += 1 ;
                for (std::size_t ti = 1; ti < modTokens.size()-1; ti++) {
                    if (modTokens[ti].type() == Opm::UDQTokenType::open_paren) noOpenPar +=1;
                    if (modTokens[ti].type() == Opm::UDQTokenType::close_paren) noOpenPar -=1;
                    if (noOpenPar <= 0) {
                        goto endLastParamBeforeLastToken;
                    }
                }
                if (modTokens.back().type() == Opm::UDQTokenType::close_paren) {
                    // remove outer set of () and repeat procedure on reduced string
                    modTokens = {modTokens.begin()+1, modTokens.end()-1};
                    noOCPPairs += 1;
                    std::cout << "modTokens " << std::endl;
                    for (std::size_t ind = 0; ind < modTokens.size(); ind++) {
                        std::cout << "ind: " << ind << "  modTokens[ind].str()   " << modTokens[ind].str() << std::endl;
                    }
                }
            }
            else {
                lastTokCP = false;
            }
        }
        endLastParamBeforeLastToken:
        return noOCPPairs;
    }

    // function to return index number of last binary token not inside bracket that is ending the expression
    std::size_t indexOfLastOp_nib(const std::vector<Opm::UDQToken>& modTokens) {
        std::size_t tok_no = 0;
        std::size_t i_start = 0;
        int noOpenPar = 0;
        std::cout << "indexOfLastOp_nib " << std::endl;
        int noOCPPairs = noOutsideOpencloseParen(modTokens);
        i_start = noOCPPairs;
        //std::size_t i_start = (modTokens[0].type() == Opm::UDQTokenType::open_paren) ? 1 : 0;
        //if ((modTokens[0].type() == Opm::UDQTokenType::open_paren) && !lastTokCP) noOpenPar = 1;
        for (std::size_t ti = i_start; ti < modTokens.size(); ti++) {
            if (modTokens[ti].type() == Opm::UDQTokenType::open_paren) noOpenPar +=1;
            if (modTokens[ti].type() == Opm::UDQTokenType::close_paren) noOpenPar -=1;
            if ((noOpenPar < 1) && (tokenTypeBinaryOp(modTokens[ti].type()) || tokenTypeFunc(modTokens[ti].type()))) tok_no = ti;
        }
        std::cout << " tok_no " << tok_no << " noOCPPairs: " << noOCPPairs << std::endl;
        return tok_no + noOCPPairs;
    }

    // function to return the precedence of the current operator/function
    int opFuncPrec(const Opm::UDQTokenType& token) {
        int prec = 0;
        if (tokenTypeFunc(token)) prec = 6;
        if (tokenTypeBinaryCmpOp(token)) prec = 5;
        if (tokenTypeBinaryPowOp(token)) prec = 4;
        if (tokenTypeBinaryMulDivOp(token)) prec = 3;
        if (tokenTypeBinaryAddSubOp(token)) prec = 2;
        if (tokenTypeBinaryUnionOp(token)) prec = 1;
        return prec;
    }

    // function to return
    //      a vector of functions and operators at the highest level plus,
    //      a map of substituted tokens
    //      the number of leading open_paren that bracket the whole expression
    struct simplifiedExpression {
        std::vector<Opm::UDQToken> highestLevOperators;
        std::map<std::size_t, std::vector<Opm::UDQToken> substitutedTokens;
        int leadingOpenPar;
    };

    struct substOuterParentheses {
        std::vector<Opm::UDQToken> highestLevOperators;
        std::vector<std::size_t> operatorPos;
        std::map<std::size_t, std::vector<Opm::UDQToken> substitutedTokens;
        int leadingOpenPar;
    };
    substOuterParentheses substitute_outer_parenthesis(const std::vector<Opm::UDQToken>& modTokens, int noLeadOpenPar) {
        std::map <std::size_t, std::vector<Opm::UDQToken> subst_tok;
        std::vector<Opm::UDQToken> high_lev_op;
        std::vector<std::size_t> operatorPos;
        std::vector<std::size_t> par_pos;
        std::vector<std::size_t> start_paren;
        std::vector<std::size_t> end_paren;
        std::size_t level = 0;
        std::size_t search_pos = 0;
        std::size_t subS_max = 0;
        int noOpenPar = 0;
        std::cout << "start handleParentheses" << std::endl;
        while (search_pos < modTokens.size()) {
            if (modTokens[search_pos].type() == Opm::UDQTokenType::open_paren  && level == 0) {
                start_paren.emplace_back(search_pos);
                level +=1;
            }
            else if (modTokens[search_pos].type() == Opm::UDQTokenType::open_paren) {
                level +=1;
            }
            else if (modTokens[search_pos].type() == Opm::UDQTokenType::close_paren && level == +1) {
                end_paren.emplace_back(search_pos);
                level -=1;
            }
            else if (modTokens[search_pos].type() == Opm::UDQTokenType::close_paren) {
                level -=1;
            }
            std::cout << " search_pos " << search_pos << " level " << level << std::endl;
            if (start_paren.size() >= 1) {
                std::cout << " start_paren " << start_paren[start_paren.end() - 1]
                          << " size_start_paren: " << start_paren.size() << std::endl;
            }
            if (end_paren.size() >= 1) {
                std::cout << " end_paren " << end_paren[end_paren.end() - 1] << " size_end_paren: " << end_paren.size()
                          << std::endl;
            }
            search_pos += 1;
        }

        // Replace content of all parentheses at the highest level by an ecl_expr
        // and store the location of all the removed parentheses

        //First store all tokens before the first start_paren
        for (std::size_t i = 0; i < start_paren[0]; i++) {
                high_lev_op.emplace_back(modTokens[i]);
            }
        for (std::size_t ind = 0; ind < start_paren.size();  ind++) {
            par_pos.emplace_back(start_paren[ind]);
            std::vector<Opm::UDQToken> substringToken;
            for (std::size_t i = start_paren[ind]; i < end_paren[ind]+1; i++) {
                substringToken.emplace_back(modTokens[search_pos]);
            }
            // store the content inside the parenthesis
            std::pair<std::size_t, std::vector<Opm::UDQToken> groupPair = std::make_pair(ind, substringToken);
            subst_tok.insert(groupPair);
            //
            // make the vector of high level tokens
            //
            //first add ecl_expr instead of content of (...)

            high_lev_op.emplace_back(Opm::UDQTokenType::ecl_expr);
            //
            // store all tokens including excluding end_paren before and start_paren after
            if (ind == start_paren.size()-1) {
                subS_max = modTokens.size();
            }
            else {
                subS_max = start_paren[ind+1];
            }
            for (std::size_t i = end_paren[ind] + 1; i < subS_max; i++) {
                high_lev_op.emplace_back(modTokens[i]);
            }
        }

        std::cout << " tok_no " << tok_no << " noOCPPairs: " << noOCPPairs << std::endl;
        return tok_no + noOCPPairs;
    }


// Categorize function in terms of which token-types are used in formula
//
// The define_type is (-) the location among a set of tokens of the "top" of the parse tree (AST - abstract syntax tree)
// i.e. the location of the lowest precedence operator relative to the total set of operators, functions and open-/close - parenthesis
    int define_type(const std::vector<Opm::UDQToken>& tokens)
    {
        int def_type = 0;
        // first token determines how the rest of the definition is handled
        bool firstTokenOpenParen = false;
        int noOpenCloseParen = noOutsideOpencloseParen(tokens);
        std::size_t ti = 0;
        // noOpenPar is a parameter that counts how many "net" open_paren has been processed
        int noOpenPar = 0;
        //
        // keep track of previous and current operator or function precedence
        int prevPrec = 100;
        int curPrec = 100;
        // branch according to formula
        //
        // find the index of the last binary operator in the expression not inside parenthesis
        std::size_t indLastBinOpTok = indexOfLastOp_nib(tokens);
        //std::cout << "indLastBinOpTok: " <<  indLastBinOpTok  <<  " token string: " << tokens[indLastBinOpTok].str() << std::endl;
        //
        if ((tokens[ti].type() == Opm::UDQTokenType::ecl_expr) || (tokens[ti].type() == Opm::UDQTokenType::number)) {
            // first token in formula is ecl_expr or number
            //std::cout << "first token- ecl_expr or number: " << tokens[ti].str() << std::endl;
            goto endTreatmentFirstToken;
        }
        else if (tokenTypeFunc(tokens[ti].type())) {
            // first token is a function
            def_type = -1;
            curPrec =  opFuncPrec(tokens[ti].type());
            prevPrec = curPrec;
            //std::cout << "first token- function: " << tokens[ti].str() << std::endl;
            //std::cout << "prevPrec " << prevPrec << "  curPrec: " << curPrec << std::endl;
        }
        else if (tokens[ti].type() == Opm::UDQTokenType::open_paren) {
            // first token is open_paren
            firstTokenOpenParen = true;
            if (noOpenCloseParen > 0) {
                if (indLastBinOpTok == 0) {
                    // adust noOpenPar for case with first token = open_paren and last token is close_paren plus no operator in between
                    noOpenPar = 0;
                    def_type = 1;
                    //std::cout << "first token- open_paren: " << tokens[ti].str() << std::endl;
                    //std::cout << "last token- close_paren: " << tokens.back().str() << std::endl;
                }
                else {
                    noOpenPar = 1 - noOpenCloseParen;
                    def_type = -1;
                    //std::cout << "first token- open_paren: " << tokens[ti].str() << "  noOpenPar: " << noOpenPar << std::endl;
                }
            }
            else {
                noOpenPar = 1;
                def_type = -1;
                //std::cout << "first token- open_paren: " << tokens[ti].str() << "  noOpenPar: " << noOpenPar << std::endl;
            }
        }
        else if (tokens[ti].type() == Opm::UDQTokenType::binary_op_sub) {
            //The minus sign in the first location is sign change - highest precedence (6)
            def_type = -1;
            curPrec =  6;
            prevPrec = curPrec;
            //std::cout << "first token- minus-sign: " << tokens[ti].str() << std::endl;
            //std::cout << "prevPrec " << prevPrec << "  curPrec: " << curPrec << std::endl;
        }
        else {
            // print error message for unhandled formula
            //std::stringstream str;
            //str << "this token cannot be handled: " << tokens[ti].str();
            //throw std::invalid_argument(str.str());
            std::cout << "this token cannot be handled: " << tokens[ti].str() << std::endl;
        }
        endTreatmentFirstToken:
        ti = 1;
        if (tokens.size()==1) {
            def_type = 1;
            return def_type;
        }
        else {
            Opm::UDQTokenType prevTokenType = tokens[0].type();
            // temporary int counter for possible intermediate operators and parentheses before next equal or lower precedence operator
            int tcount = 0;
            while (ti < indLastBinOpTok+1) {
                // treat ecl_expr
                    if ((tokens[ti].type() == Opm::UDQTokenType::ecl_expr) || (tokens[ti].type() == Opm::UDQTokenType::number)) {
                        //std::cout << "token no: " << ti << "  ecl_expr or number: " << tokens[ti].str() << std::endl;
                        prevTokenType = tokens[ti].type();
                        ti += 1;
                    }
                    else if (tokens[ti].type() == Opm::UDQTokenType::open_paren) {
                        //std::cout << "token no: " << ti << " open_paren: " << tokens[ti].str() << std::endl;
                        noOpenPar += 1;
                        tcount += 1;
                        prevTokenType = tokens[ti].type();
                        ti += 1;
                    }
                    else if (tokens[ti].type() == Opm::UDQTokenType::close_paren) {
                        //std::cout << "token no: " << ti << "  close_paren: " << tokens[ti].str() << std::endl;
                        noOpenPar -= 1;
                        tcount += 1;
                        prevTokenType = tokens[ti].type();
                        ti += 1;
                    }
                    else if (tokenTypeBinaryOp(tokens[ti].type()) || tokenTypeFunc(tokens[ti].type())) {
                        //
                        // update the previous operator precedence only if outside all parenthesis
                        //std::cout << "token no: " << ti << "  bin_op or function " << tokens[ti].str() << std::endl;
                        //
                        if (noOpenPar <= 0) {
                            //check if operator is a change sign operator - if so set appropriate precedence level and increment
                            if ((tokens[ti].type() == Opm::UDQTokenType::binary_op_sub) && prevTokenType == Opm::UDQTokenType::open_paren) {
                                curPrec = 6;
                            }
                            else {
                                curPrec =  opFuncPrec(tokens[ti].type());
                            }
                            //std::cout << " Outside parenthesis - prevPrec " << prevPrec << "  curPrec: " << tokens[ti].str()  << curPrec << std::endl;
                            if (curPrec <= prevPrec) {
                                // if current precedence is lower than previous add minus (tcount + 1) to def_type and find next operator/function
                                //
                                //std::cout << " LowerOrEqual: prevPrec " << prevPrec << "  curPrec: " << curPrec << tokens[ti].str() << std::endl;
                                def_type -= tcount + 1;
                                tcount = 0;
                                prevTokenType = tokens[ti].type();
                                prevPrec = curPrec;
                                ti += 1;
                            }
                            else {
                                // found operator / function with higher precedence - increment tcount and ti
                                //std::cout << " Higher: prevPrec  " << prevPrec << "  curPrec:  " << curPrec << tokens[ti].str() << std::endl;
                                tcount += 1;
                                ti += 1;
                            }
                        }
                        else {
                        // current token is inside one or more prentheses - increment def_type until outside
                            //std::cout << "inside parenthesis - token no: " << ti << "   " << tokens[ti].str() << std::endl;
                            tcount += 1;
                            prevTokenType = tokens[ti].type();
                            ti += 1;
                        }
                    }
                    else {
                        // print error message for unhandled formula
                        //std::stringstream str;
                        //str << "this token cannot be handled: " << tokens[ti].str();
                        //throw std::invalid_argument(str.str());
                        std::cout << "this token cannot be handled: " << tokens[ti].str() << std::endl;
                        ti = indLastBinOpTok+1;
                    }

                }
//            }
        return def_type;
        }
    }

    namespace iUdq {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            int nwin = std::max(udqDims[0], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[1]) }
            };
        }

        template <class IUDQArray>
        void staticContrib(const Opm::UDQInput& udq_input, IUDQArray& iUdq)
        {
            if (udq_input.is<Opm::UDQDefine>()) {
                const auto& udq_define = udq_input.get<Opm::UDQDefine>();
                const auto& tokens = udq_define.tokens();
#if 0
                std::cout << "UDQ-DEFINE - keyword: " << udq_define.keyword() << std::endl;
                std::size_t icnt = 0;
                for (auto it = tokens.begin(); it != tokens.end(); it++) {
                    icnt+=1;
                    const int s_key = static_cast<int>(*it);
                    std::cout << "func_token no: " << icnt << "  token type: " << s_key << std::endl;
                }
                icnt = 0;
                double val = 0.;
                std::string val_string = "";
                for (const auto& token : all_tokens) {
                    icnt+=1;
                    val = 0.;
                    val_string = "";
                    try {
                        val = std::get<double>(token.value());
                    }
                    catch (const std::bad_variant_access&) {
                        val_string = std::get<std::string>(token.value());
                        std::cout << "token: " << token.str() << " no double value" << std::endl;
                    }
                    const int token_type = static_cast<int>(token.type());

                    std::cout << "token no: " << icnt << "  token type: " << token_type << " token_string: " << token.str() << std::endl;
                    std::cout << " value_string " << val_string << " value_double: " << val << std::endl;
                }
#endif
                iUdq[0] = 2;
                iUdq[1] = define_type(tokens);
            } else {
                iUdq[0] = 0;
                iUdq[1] = 0;
            }
            iUdq[2] = udq_input.index.typed_insert_index;
        }

    } // iUdq

    namespace iUad {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            int nwin = std::max(udqDims[2], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[3]) }
            };
        }

        template <class IUADArray>
        void staticContrib(const Opm::UDQActive::Record& udq_record, IUADArray& iUad, int use_cnt_diff)
        {
            iUad[0] = udq_record.uad_code;
            iUad[1] = udq_record.input_index + 1;

            // entry 3  - unknown meaning - value = 1
            iUad[2] = 1;

            iUad[3] = udq_record.use_count;
            iUad[4] = udq_record.use_index + 1 - use_cnt_diff;
        }
    } // iUad


    namespace zUdn {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>>;
            int nwin = std::max(udqDims[0], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[4]) }
            };
        }

    template <class zUdnArray>
    void staticContrib(const Opm::UDQInput& udq_input, zUdnArray& zUdn)
    {
        // entry 1 is udq keyword
        zUdn[0] = udq_input.keyword();
        zUdn[1] = udq_input.unit();
    }
    } // zUdn

    namespace zUdl {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>>;
            int nwin = std::max(udqDims[0], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[5]) }
            };
        }

    template <class zUdlArray>
    void staticContrib(const Opm::UDQInput& input, zUdlArray& zUdl)
        {
            int l_sstr = 8;
            int max_l_str = 128;
            std::string temp_str = "";
            // write out the input formula if key is a DEFINE udq
            if (input.is<Opm::UDQDefine>()) {
                const auto& udq_define = input.get<Opm::UDQDefine>();
                const std::string& z_data = udq_define.input_string();
                int n_sstr =  z_data.size()/l_sstr;
                if (static_cast<int>(z_data.size()) > max_l_str) {
                    std::cout << "Too long input data string (max 128 characters): " << z_data << std::endl;
                    throw std::invalid_argument("UDQ - variable: " + udq_define.keyword());
                }
                else {
                    for (int i = 0; i < n_sstr; i++) {
                        if (i == 0) {
                            temp_str = z_data.substr(i*l_sstr, l_sstr);
                            //if first character is a minus sign, change to ~
                            if (temp_str.compare(0,1,"-") == 0) {
                                temp_str.replace(0,1,"~");
                            }
                            zUdl[i] = temp_str;
                        }
                        else {
                            zUdl[i] = z_data.substr(i*l_sstr, l_sstr);
                        }
                    }
                    //add remainder of last non-zero string
                    if ((z_data.size() % l_sstr) > 0)
                        zUdl[n_sstr] = z_data.substr(n_sstr*l_sstr);
                }
            }
        }
    } // zUdl

    namespace iGph {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(udqDims[6]) },
                WV::WindowSize{ static_cast<std::size_t>(1) }
            };
        }

        template <class IGPHArray>
        void staticContrib(const int    inj_phase,
                           IGPHArray&   iGph)
        {
                iGph[0] = inj_phase;
        }
    } // iGph

    namespace iUap {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            int nwin = std::max(udqDims[7], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(1) }
            };
        }

        template <class IUAPArray>
        void staticContrib(const int    wg_no,
                           IUAPArray&   iUap)
        {
                iUap[0] = wg_no+1;
        }
    } // iUap

    namespace dUdw {

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;
            int nwin = std::max(udqDims[9], 1);
            int nitPrWin = std::max(udqDims[8], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

        template <class DUDWArray>
        void staticContrib(const Opm::UDQState& udq_state,
                           const std::vector<std::string>& wnames,
                           const std::string udq,
                           const std::size_t nwmaxz,
                           DUDWArray&   dUdw)
        {
            //initialize array to the default value for the array
            for (std::size_t ind = 0; ind < nwmaxz; ind++) {
                dUdw[ind] = Opm::UDQ::restart_default;
            }
            for (std::size_t ind = 0; ind < wnames.size(); ind++) {
                if (udq_state.has_well_var(wnames[ind], udq)) {
                    dUdw[ind] = udq_state.get_well_var(wnames[ind], udq);
                }
            }
        }
    } // dUdw

        namespace dUdg {

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;
            int nwin = std::max(udqDims[11], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(udqDims[10]) }
            };
        }

        template <class DUDGArray>
        void staticContrib(const Opm::UDQState& udq_state,
                           const std::vector<const Opm::Group*> groups,
                           const std::string udq,
                           const std::size_t ngmaxz,
                           DUDGArray&   dUdg)
        {
            //initialize array to the default value for the array
            for (std::size_t ind = 0; ind < groups.size(); ind++) {
                if ((groups[ind] == nullptr) || (ind == ngmaxz-1)) {
                    dUdg[ind] = Opm::UDQ::restart_default;
                }
                else {
                    if (udq_state.has_group_var((*groups[ind]).name(), udq)) {
                        dUdg[ind] = udq_state.get_group_var((*groups[ind]).name(), udq);
                    }
                    else {
                        dUdg[ind] = Opm::UDQ::restart_default;
                    }
                }
            }
        }
    } // dUdg

        namespace dUdf {

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const std::vector<int>& udqDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;
            int nwin = std::max(udqDims[12], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(1) }
            };
        }

        template <class DUDFArray>
        void staticContrib(const Opm::UDQState& udq_state,
                           const std::string udq,
                           DUDFArray&   dUdf)
        {
            //set value for group name "FIELD"
            if (udq_state.has(udq)) {
                dUdf[0] = udq_state.get(udq);
            }
            else {
                dUdf[0] = Opm::UDQ::restart_default;
            }
        }
    } // dUdf
}


// =====================================================================


template < typename T>
std::pair<bool, int > findInVector(const std::vector<T>  & vecOfElements, const T  & element)
{
    std::pair<bool, int > result;

    // Find given element in vector
    auto it = std::find(vecOfElements.begin(), vecOfElements.end(), element);

    if (it != vecOfElements.end())
    {
        result.second = std::distance(vecOfElements.begin(), it);
        result.first = true;
    }
    else
    {
        result.first = false;
        result.second = -1;
    }
    return result;
}


const std::vector<int> Opm::RestartIO::Helpers::igphData::ig_phase(const Opm::Schedule& sched,
                                                                   const std::size_t simStep,
                                                                   const std::vector<int>& inteHead )
{
    const auto curGroups = sched.restart_groups(simStep);
    std::vector<int> inj_phase(ngmaxz(inteHead), 0);
    for (std::size_t ind = 0; ind < curGroups.size(); ind++) {
        if (curGroups[ind] != nullptr) {
            const auto& group = *curGroups[ind];
            if (group.isInjectionGroup()) {
                /*
                  Initial code could only inject one phase for each group, then
                  numerical value '3' was used for the gas phase, that can not
                  be right?
                */
                int phase_sum = 0;
                if (group.hasInjectionControl(Opm::Phase::OIL))
                    phase_sum += 1;
                if (group.hasInjectionControl(Opm::Phase::WATER))
                    phase_sum += 2;
                if (group.hasInjectionControl(Opm::Phase::GAS))
                    phase_sum += 4;
                inj_phase[group.insert_index()] = phase_sum;
            }
        }
    }
    return inj_phase;
}

const std::vector<int> iuap_data(const Opm::Schedule& sched,
                                    const std::size_t simStep,
                                    const std::vector<Opm::UDQActive::InputRecord>& iuap)
{
    //construct the current list of well or group sequence numbers to output the IUAP array
    std::vector<int> wg_no;
    Opm::UDAKeyword wg_key;

    for (std::size_t ind = 0; ind < iuap.size(); ind++) {
        auto& ctrl = iuap[ind].control;
        wg_key = Opm::UDQ::keyword(ctrl);
        if ((wg_key == Opm::UDAKeyword::WCONPROD) || (wg_key == Opm::UDAKeyword::WCONINJE)) {
            const auto& well = sched.getWell(iuap[ind].wgname, simStep);
            wg_no.push_back(well.seqIndex());
        }
        else if ((wg_key == Opm::UDAKeyword::GCONPROD) || (wg_key == Opm::UDAKeyword::GCONINJE)) {
            const auto& group = sched.getGroup(iuap[ind].wgname, simStep);
            if (iuap[ind].wgname != "FIELD") {
                wg_no.push_back(group.insert_index() - 1);
            }
        }
        else {
            std::cout << "Invalid Control keyword: " << static_cast<int>(ctrl) << std::endl;
            throw std::invalid_argument("UDQ - variable: " + iuap[ind].udq );
        }

    }

    return wg_no;
}

Opm::RestartIO::Helpers::AggregateUDQData::
AggregateUDQData(const std::vector<int>& udqDims)
    : iUDQ_ (iUdq::allocate(udqDims)),
      iUAD_ (iUad::allocate(udqDims)),
      zUDN_ (zUdn::allocate(udqDims)),
      zUDL_ (zUdl::allocate(udqDims)),
      iGPH_ (iGph::allocate(udqDims)),
      iUAP_ (iUap::allocate(udqDims)),
      dUDW_ (dUdw::allocate(udqDims)),
      dUDG_ (dUdg::allocate(udqDims)),
      dUDF_ (dUdf::allocate(udqDims))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateUDQData::
captureDeclaredUDQData(const Opm::Schedule&                 sched,
                       const std::size_t                    simStep,
                       const Opm::UDQState&                 udq_state,
                       const std::vector<int>&              inteHead)
{
    const auto& udqCfg = sched.getUDQConfig(simStep);
    const auto nudq = inteHead[VI::intehead::NO_WELL_UDQS] + inteHead[VI::intehead::NO_GROUP_UDQS] + inteHead[VI::intehead::NO_FIELD_UDQS];
    int cnt_udq = 0;
    for (const auto& udq_input : udqCfg.input()) {
        auto udq_index = udq_input.index.insert_index;
        {
            auto i_udq = this->iUDQ_[udq_index];
            iUdq::staticContrib(udq_input, i_udq);
        }
        {
            auto z_udn = this->zUDN_[udq_index];
            zUdn::staticContrib(udq_input, z_udn);
        }
        {
            auto z_udl = this->zUDL_[udq_index];
            zUdl::staticContrib(udq_input, z_udl);
        }
        cnt_udq += 1;
    }
    if (cnt_udq != nudq) {
        std::stringstream str;
        str << "Inconsistent total number of udqs: " << cnt_udq << " and sum of well, group and field udqs: " << nudq;
        OpmLog::error(str.str());
    }


    auto udq_active = sched.udqActive(simStep);
    if (udq_active) {
        const auto& udq_records = udq_active.get_iuad();
        int cnt_iuad = 0;
        for (std::size_t index = 0; index < udq_records.size(); index++) {
            const auto& record = udq_records[index];
            auto i_uad = this->iUAD_[cnt_iuad];
            const auto& ctrl = record.control;
            const auto wg_key = Opm::UDQ::keyword(ctrl);
            if (!(((wg_key == Opm::UDAKeyword::GCONPROD) || (wg_key == Opm::UDAKeyword::GCONINJE)) && (record.wg_name() == "FIELD"))) {
                int use_count_diff = static_cast<int>(index) - cnt_iuad;
                iUad::staticContrib(record, i_uad, use_count_diff);
                cnt_iuad += 1;
            }
        }
        if (cnt_iuad != inteHead[VI::intehead::NO_IUADS]) {
            std::stringstream str;
            str << "Inconsistent number of iuad's: " << cnt_iuad << " number of iuad's from intehead " << inteHead[VI::intehead::NO_IUADS];
            OpmLog::error(str.str());
        }

        const auto& iuap_records = udq_active.get_iuap();
        int cnt_iuap = 0;
        const auto iuap_vect = iuap_data(sched, simStep,iuap_records);
        for (std::size_t index = 0; index < iuap_vect.size(); index++) {
            const auto& wg_no = iuap_vect[index];
            auto i_uap = this->iUAP_[index];
            iUap::staticContrib(wg_no, i_uap);
            cnt_iuap += 1;
        }
        if (cnt_iuap != inteHead[VI::intehead::NO_IUAPS]) {
            std::stringstream str;
            str << "Inconsistent number of iuap's: " << cnt_iuap << " number of iuap's from intehead " << inteHead[VI::intehead::NO_IUAPS];
            OpmLog::error(str.str());
        }


    }
    if (inteHead[VI::intehead::NO_GROUP_UDQS] > 0) {
        Opm::RestartIO::Helpers::igphData igph_dat;
        int cnt_igph = 0;
        auto igph = igph_dat.ig_phase(sched, simStep, inteHead);
        for (std::size_t index = 0; index < igph.size(); index++) {
                auto i_igph = this->iGPH_[index];
                iGph::staticContrib(igph[index], i_igph);
                cnt_igph += 1;
        }
        if (cnt_igph != inteHead[VI::intehead::NGMAXZ]) {
            std::stringstream str;
            str << "Inconsistent number of igph's: " << cnt_igph << " number of igph's from intehead " << inteHead[VI::intehead::NGMAXZ];
            OpmLog::error(str.str());
        }
    }

    std::size_t i_wudq = 0;
    const auto& wnames = sched.wellNames(simStep);
    const auto nwmax = nwmaxz(inteHead);
    int cnt_dudw = 0;
    for (const auto& udq_input : udqCfg.input()) {
        if (udq_input.var_type() ==  UDQVarType::WELL_VAR) {
            const std::string& udq = udq_input.keyword();
            auto i_dudw = this->dUDW_[i_wudq];
            dUdw::staticContrib(udq_state, wnames, udq, nwmax, i_dudw);
            i_wudq++;
            cnt_dudw += 1;
        }
    }
    if (cnt_dudw != inteHead[VI::intehead::NO_WELL_UDQS]) {
        std::stringstream str;
        str << "Inconsistent number of dudw's: " << cnt_dudw << " number of dudw's from intehead " << inteHead[VI::intehead::NO_WELL_UDQS];
        OpmLog::error(str.str());
    }

    std::size_t i_gudq = 0;
    const auto curGroups = sched.restart_groups(simStep);
    const auto ngmax = ngmaxz(inteHead);
    int cnt_dudg = 0;
    for (const auto& udq_input : udqCfg.input()) {
        if (udq_input.var_type() ==  UDQVarType::GROUP_VAR) {
            const std::string& udq = udq_input.keyword();
            auto i_dudg = this->dUDG_[i_gudq];
            dUdg::staticContrib(udq_state, curGroups, udq, ngmax, i_dudg);
            i_gudq++;
            cnt_dudg += 1;
        }
    }
    if (cnt_dudg != inteHead[VI::intehead::NO_GROUP_UDQS]) {
        std::stringstream str;
        str << "Inconsistent number of dudg's: " << cnt_dudg << " number of dudg's from intehead " << inteHead[VI::intehead::NO_GROUP_UDQS];
        OpmLog::error(str.str());
    }

    std::size_t i_fudq = 0;
    int cnt_dudf = 0;
    for (const auto& udq_input : udqCfg.input()) {
        if (udq_input.var_type() ==  UDQVarType::FIELD_VAR) {
            const std::string& udq = udq_input.keyword();
            auto i_dudf = this->dUDF_[i_fudq];
            dUdf::staticContrib(udq_state, udq, i_dudf);
            i_fudq++;
            cnt_dudf += 1;
        }
    }
    if (cnt_dudf != inteHead[VI::intehead::NO_FIELD_UDQS]) {
        std::stringstream str;
        str << "Inconsistent number of dudf's: " << cnt_dudf << " number of dudf's from intehead " << inteHead[VI::intehead::NO_FIELD_UDQS];
        OpmLog::error(str.str());
    }


}


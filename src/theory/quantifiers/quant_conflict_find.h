/*********************                                                        */
/*! \file quant_conflict_find.h
 ** \verbatim
 ** Original author: Andrew Reynolds
 ** Major contributors: none
 ** Minor contributors (to current version): none
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief quantifiers conflict find class
 **/

#include "cvc4_private.h"

#ifndef QUANT_CONFLICT_FIND
#define QUANT_CONFLICT_FIND

#include "context/cdhashmap.h"
#include "context/cdchunk_list.h"
#include "theory/quantifiers_engine.h"

namespace CVC4 {
namespace theory {
namespace quantifiers {

class QcfNode;

class QuantConflictFind;

class QcfNodeIndex {
public:
  std::map< TNode, QcfNodeIndex > d_children;
  void clear() { d_children.clear(); }
  void debugPrint( const char * c, int t );
  Node existsTerm( TNode n, std::vector< TNode >& reps, int index = 0 );
  Node addTerm( TNode n, std::vector< TNode >& reps, int index = 0 );
};

class QuantInfo;

//match generator
class MatchGen {
private:
  //current children information
  int d_child_counter;
  //children of this object
  //std::vector< int > d_children_order;
  unsigned getNumChildren() { return d_children.size(); }
  //MatchGen * getChild( int i ) { return &d_children[d_children_order[i]]; }
  MatchGen * getChild( int i ) { return &d_children[i]; }
  //current matching information
  std::vector< QcfNodeIndex * > d_qn;
  std::vector< std::map< TNode, QcfNodeIndex >::iterator > d_qni;
  bool doMatching( QuantConflictFind * p, QuantInfo * qi );
  //for matching : each index is either a variable or a ground term
  unsigned d_qni_size;
  std::map< int, int > d_qni_var_num;
  std::map< int, TNode > d_qni_gterm;
  std::map< int, TNode > d_qni_gterm_rep;
  std::map< int, int > d_qni_bound;
  std::map< int, TNode > d_qni_bound_cons;
  std::map< int, int > d_qni_bound_cons_var;
  std::map< int, int >::iterator d_binding_it;
  bool d_binding;
  //int getVarBindingVar();
  std::map< int, Node > d_ground_eval;
public:
  //type of the match generator
  enum {
    typ_invalid,
    typ_ground,
    typ_pred,
    typ_eq,
    typ_formula,
    typ_var,
    typ_top,
  };
  void debugPrintType( const char * c, short typ, bool isTrace = false );
public:
  MatchGen() : d_type( typ_invalid ){}
  MatchGen( QuantInfo * qi, Node n, bool isTop = false, bool isVar = false );
  bool d_tgt;
  Node d_n;
  std::vector< MatchGen > d_children;
  short d_type;
  bool d_type_not;
  void reset_round( QuantConflictFind * p );
  void reset( QuantConflictFind * p, bool tgt, QuantInfo * qi );
  bool getNextMatch( QuantConflictFind * p, QuantInfo * qi );
  bool isValid() { return d_type!=typ_invalid; }
  void setInvalid();

  // is this term treated as UF application?
  static bool isHandledUfTerm( TNode n );
  static Node getFunction( Node n );
};

//info for quantifiers
class QuantInfo {
private:
  void registerNode( Node n, bool hasPol, bool pol );
  void flatten( Node n );
  //the variables that this quantified formula has not beneath nested quantifiers
  std::map< TNode, bool > d_has_var;
public:
  QuantInfo() : d_mg( NULL ) {}
  std::vector< TNode > d_vars;
  std::map< TNode, int > d_var_num;
  //std::map< EqRegistry *, bool > d_rel_eqr;
  std::map< int, std::vector< Node > > d_var_constraint[2];
  int getVarNum( TNode v ) { return d_var_num.find( v )!=d_var_num.end() ? d_var_num[v] : -1; }
  bool isVar( TNode v ) { return d_var_num.find( v )!=d_var_num.end(); }
  int getNumVars() { return (int)d_vars.size(); }
  TNode getVar( int i ) { return d_vars[i]; }
  MatchGen * d_mg;
  Node d_q;
  std::map< int, MatchGen * > d_var_mg;
  void reset_round( QuantConflictFind * p );
public:
  //initialize
  void initialize( Node q, Node qn );
  //current constraints
  std::vector< TNode > d_match;
  std::vector< TNode > d_match_term;
  std::map< int, std::map< TNode, int > > d_curr_var_deq;
  int getCurrentRepVar( int v );
  TNode getCurrentValue( TNode n );
  bool getCurrentCanBeEqual( QuantConflictFind * p, int v, TNode n, bool chDiseq = false );
  int addConstraint( QuantConflictFind * p, int v, TNode n, bool polarity );
  int addConstraint( QuantConflictFind * p, int v, TNode n, int vn, bool polarity, bool doRemove );
  bool setMatch( QuantConflictFind * p, int v, TNode n );
  bool isMatchSpurious( QuantConflictFind * p );
  bool completeMatch( QuantConflictFind * p, std::vector< int >& assigned );
  void debugPrintMatch( const char * c );
  bool isConstrainedVar( int v );
};

class QuantConflictFind : public QuantifiersModule
{
  friend class QcfNodeIndex;
  friend class MatchGen;
  friend class QuantInfo;
  typedef context::CDChunkList<Node> NodeList;
  typedef context::CDHashMap<Node, bool, NodeHashFunction> NodeBoolMap;
private:
  context::Context* d_c;
  context::CDO< bool > d_conflict;
  bool d_performCheck;
  //void registerAssertion( Node n );
private:
  std::map< Node, Node > d_op_node;
  int d_fid_count;
  std::map< Node, int > d_fid;
  Node mkEqNode( Node a, Node b );
public:  //for ground terms
  Node d_true;
  Node d_false;
private:
  Node evaluateTerm( Node n );
  int evaluate( Node n, bool pref = false, bool hasPref = false );
private:
  //currently asserted quantifiers
  NodeList d_qassert;
  std::map< Node, QuantInfo > d_qinfo;
private:  //for equivalence classes
  eq::EqualityEngine * getEqualityEngine();
  bool areDisequal( Node n1, Node n2 );
  bool areEqual( Node n1, Node n2 );
  Node getRepresentative( Node n );

/*
  class EqcInfo {
  public:
    EqcInfo( context::Context* c ) : d_diseq( c ) {}
    NodeBoolMap d_diseq;
    bool isDisequal( Node n ) { return d_diseq.find( n )!=d_diseq.end() && d_diseq[n]; }
    void setDisequal( Node n, bool val = true ) { d_diseq[n] = val; }
    //NodeBoolMap& getRelEqr( int index ) { return index==0 ? d_rel_eqr_e : d_rel_eqr_d; }
  };
  std::map< Node, EqcInfo * > d_eqc_info;
  EqcInfo * getEqcInfo( Node n, bool doCreate = true );
*/
  // operator -> index(terms)
  std::map< TNode, QcfNodeIndex > d_uf_terms;
  // operator -> index(eqc -> terms)
  std::map< TNode, QcfNodeIndex > d_eqc_uf_terms;
  //get qcf node index
  QcfNodeIndex * getQcfNodeIndex( Node eqc, Node f );
  QcfNodeIndex * getQcfNodeIndex( Node f );
  // type -> list(eqc)
  std::map< TypeNode, std::vector< TNode > > d_eqcs;
  //mapping from UF terms to representatives of their arguments
  std::map< TNode, std::vector< TNode > > d_arg_reps;
  //compute arg reps
  void computeArgReps( TNode n );
public:
  enum {
    effort_conflict,
    effort_prop_eq,
    effort_prop_deq,
  };
  short d_effort;
  //for effort_prop
  TNode d_prop_eq[2];
  bool d_prop_pol;
  bool isPropagationSet();
  bool areMatchEqual( TNode n1, TNode n2 );
  bool areMatchDisequal( TNode n1, TNode n2 );
public:
  QuantConflictFind( QuantifiersEngine * qe, context::Context* c );

  /** register assertions */
  //void registerAssertions( std::vector< Node >& assertions );
  /** register quantifier */
  void registerQuantifier( Node q );

public:
  /** assert quantifier */
  void assertNode( Node q );
  /** new node */
  void newEqClass( Node n );
  /** merge */
  void merge( Node a, Node b );
  /** assert disequal */
  void assertDisequal( Node a, Node b );
  /** check */
  void check( Theory::Effort level );
  /** needs check */
  bool needsCheck( Theory::Effort level );
private:
  void computeRelevantEqr();
private:
  void debugPrint( const char * c );
  //for debugging
  std::vector< Node > d_quants;
  std::map< Node, int > d_quant_id;
  void debugPrintQuant( const char * c, Node q );
  void debugPrintQuantBody( const char * c, Node q, Node n, bool doVarNum = true );
public:
  /** statistics class */
  class Statistics {
  public:
    IntStat d_inst_rounds;
    IntStat d_conflict_inst;
    IntStat d_prop_inst;
    Statistics();
    ~Statistics();
  };
  Statistics d_statistics;
};

}
}
}

#endif

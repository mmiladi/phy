/******************************************************************** 
 * Copyright (C) 2008-2014 Jakob Skou Pedersen - All Rights Reserved.
 *
 * See README_license.txt for license agreement.
 *******************************************************************/
#include "DiscreteFactorGraph.h"

namespace phy {

  DFGNode::DFGNode(unsigned dimension) : 
  isFactor(false), dimension(dimension) {}


  DFGNode::DFGNode(xmatrix_t const & potential) : 
    isFactor(true), potential(potential)
    {
      if (potential.size1() == 1)
	dimension = 1;
      else
	dimension = 2;
    }

  // The graph is defined in terms of the factor neighbors. We want
  // links to be represented both ways, i.e., also from var nodes to
  // fac nodes. This function returns the neighbors from the var point
  // of view.

  // precondition: neighbors harbor only one-way links
  void addTwoWayLinks(vector<vector<unsigned> > & neighbors)
  {
    vector<vector<unsigned> > ref(neighbors);
    for (unsigned i = 0; i < ref.size(); i++) 
      for (unsigned j = 0; j < ref[i].size(); j++) {
	unsigned other = ref[i][j];
	neighbors[other].push_back(i);
      }
  }


  DFG::DFG(vector<unsigned> const & varDimensions, 
	   vector<xmatrix_t> const & facPotentials, 
	   vector<vector<unsigned> > const & facNeighbors)
  {
    init(varDimensions, facPotentials, facNeighbors);
  }


#ifndef XNUMBER_IS_NUMBER
  DFG::DFG(vector<unsigned> const & varDimensions, 
	   vector<matrix_t> const & facPotentials, 
	   vector<vector<unsigned> > const & facNeighbors)
  {
    vector<xmatrix_t> v;
    BOOST_FOREACH(matrix_t const & m, facPotentials)
      v.push_back( toXNumber(m) );
    init(varDimensions, v, facNeighbors);
  }
#endif


  void DFG::resetFactorPotential(xmatrix_t const & pot, unsigned facId)
  {
    DFGNode & nd = nodes[ convFacToNode(facId) ];
    if( nd.potential.size1() != pot.size1() )
      errorAbort("DFG::resetFactorPotential: nd.potential.size1()="+toString(nd.potential.size1())+" does not match pot.size1()="+toString(pot.size1()) );
    if( nd.potential.size2() != pot.size2() )
      errorAbort("DFG::resetFactorPotential: nd.potential.size2()="+toString(nd.potential.size2())+" does not match pot.size2()="+toString(pot.size2()) );
    nd.potential = pot;
  }


  void DFG::resetFactorPotentials(vector<xmatrix_t> const & facPotVecSubSet, vector<unsigned> const & facMap)
  {
    assert(facPotVecSubSet.size() == facMap.size() );
    for (unsigned i = 0; i < facMap.size(); i++)
      resetFactorPotential( facPotVecSubSet[i], facMap[i] );
  }


  void DFG::resetFactorPotentials(vector<xmatrix_t> const & facPotVec)
  {
    assert(facPotVec.size() == factors.size() );
    for (unsigned i = 0; i < facPotVec.size(); i++)
      resetFactorPotential(facPotVec[i], i);
  }


#ifndef XNUMBER_IS_NUMBER
  void DFG::resetFactorPotential(matrix_t const & facPotVec, unsigned facId)
  {
    resetFactorPotential( toXNumber(facPotVec), facId );
  }


  void DFG::resetFactorPotentials(vector<matrix_t> const & facPotVecSubSet, vector<unsigned> const & facMap)
  {
    assert(facPotVecSubSet.size() == facMap.size() );
    for (unsigned i = 0; i < facMap.size(); i++)
      resetFactorPotential(facPotVecSubSet[i], facMap[i]);
  }


  void DFG::resetFactorPotentials(vector<matrix_t> const & facPotVec)
  {
    assert(facPotVec.size() == factors.size() );
    for (unsigned i = 0; i < facPotVec.size(); i++)
      resetFactorPotential(toXNumber(facPotVec[i]), i);
  }
#endif


  // Begining of  write dot

  /** functor for use with write_graphviz. Writes out internal properties */
  struct DFGNodeWriter 
  {
    DFGNodeWriter(DFG const & fg) : fg_ (fg) {};

    // writing factor node in dot format
    void mkDotFacNode(std::ostream& out, unsigned const & v)
    {
      out << " [label=\"f" << fg_.convNodeToFac(v) << "\\n" << v << "\", shape=box]" << endl;
    }

    // writing variable node in dot format
    void mkDotVarNode(std::ostream& out, unsigned const & v)
    {
      out << " [label=\"v" << fg_.convNodeToVar(v) << "\\n" << v << "\"]" << endl;
    }
    
    template <class Vertex>
    void operator()(ostream & out, Vertex const & v) 
    {
      if (fg_.nodes[v].isFactor) 
	mkDotFacNode(out, v);
      else
	mkDotVarNode(out, v);
    }

 
    DFG const & fg_;
  };


  boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> DFG::mkBoostGraph()
  {
    set< pair<unsigned, unsigned> > seen;
    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> bfg( nodes.size() );
    for (unsigned i = 0; i < neighbors.size(); i++)
      for (unsigned j = 0; j < neighbors[i].size(); j++) {
	unsigned from = i;
	unsigned to = neighbors[i][j];
	if(seen.insert(pair<unsigned, unsigned>(from, to) ).second and seen.insert(pair<unsigned, unsigned>(to, from) ).second ) // link not seen 
	  add_edge(i, neighbors[i][j], bfg);
      }
    return bfg;
  }


  string DFG::writeDot()
  {
    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> bfg( mkBoostGraph() );
    stringstream ss;
    write_graphviz(ss, bfg, DFGNodeWriter(*this) );
    return ss.str();
  }


  void DFG::writeDot(string const &fileName)
  {
    ofstream f(fileName.c_str(), ios::out);
    if (!f)
      errorAbort("Cannot open file: " + fileName + "\n");
  
    f << writeDot();
    f.close();
  }

  // end of write dot

  void DFG::writeInfo(ostream & str, vector<string> const & varNames, vector<string> const & facNames)
  {
    str << "Variable info:" << endl << endl;
    for (unsigned i = 0; i < variables.size(); i++)
      str << variableInfoStr( i, varNames, facNames) << endl;
    str << endl;

    str << "Factor info:" << endl << endl;
    for (unsigned i = 0; i < factors.size(); i++)
      str << factorInfoStr(i, varNames, facNames) << endl;
    str << endl;
  }


  string DFG::factorInfoStr(unsigned const i, vector<string> varNames, vector<string> facNames)
  {
    bool useFacNames = (facNames.size() > 0) ? true : false;
    bool useVarNames = (varNames.size() > 0) ? true : false;
    string s = 
      "Factor index: " + toString(i) 
      + ( useFacNames  ? ("\t" + facNames[i] ) : "") + "\n"
      + "Node index: " + toString( convFacToNode(i) ) + "\n"
      + "Variable neighbors: " + toString( getFactorNeighbors(i) )
      + ( useVarNames ? ("\t" + toString( mkSubset( varNames, getFactorNeighbors(i) ) ) ) : "")  + "\n" // exploiting that node and var ids are equal
      + "Dimension: " + toString( getFactor(i).dimension ) + "\n"
      + "Potential: ";
    std::stringstream ss;
    ss << getFactor(i).potential << endl;
    s += ss.str();
    return s;
  }


  string DFG::variableInfoStr(unsigned const i, vector<string> varNames, vector<string> facNames)
  {
    bool useFacNames = (facNames.size() > 0) ? true : false;
    bool useVarNames = (varNames.size() > 0) ? true : false;
    vector<unsigned> facSubset;
    if (useFacNames) {
      BOOST_FOREACH(unsigned j, getVariableNeighbors(i) )
	facSubset.push_back( convNodeToFac(j) );
    }
    string s = 
      "Variable index: " + toString(i) 
      + ( useVarNames  ? ("\t" + varNames[i] ) : " ") + "\n"
      + "Node index: " + toString( convVarToNode(i) ) + "\n"
      + "Factor neighbors: " + toString( getVariableNeighbors(i) ) 
      + ( useFacNames ? ("\t" + toString( mkSubset( facNames, facSubset ) ) ) : "")  + "\n" 
      + "Dimension: " + toString( getVariable(i).dimension ) + "\n";
    return s;
  }


  void DFG::consistencyCheck()
  {
    // check factor dimension and number of neighbors
    for (unsigned i = 0; i < factors.size(); i++) 
      if (getFactorNeighbors(i).size() != getFactor(i).dimension)
	errorAbort("Inconsistent graph. \nNumber of neighbors and potential dimenension does not match at factor:\n\n" + factorInfoStr(i) );
    
    // check that factor potential dimensions equal dimension of neigboring var nodes
    for (unsigned i = 0; i < factors.size(); i++) {
      vector<unsigned> nbs = getFactorNeighbors(i);
      unsigned nbCount = nbs.size();
      if (nbCount > 0) {
	unsigned nb = nbs[0];
	if ( ( nbCount != 1 and getFactor(i).potential.size1() != getVariable(nb).dimension ) or (nbCount == 1 and getFactor(i).potential.size2() != getVariable(nb).dimension) ) // if ncCount equals 1 then the potential is a prior
	  errorAbort("Inconsistent graph. \nFactor potential dimensions does not match dimension of neighboring variable node (both defined below):\n\n" + factorInfoStr(i)  + "\n" + variableInfoStr(i) + "\n\n" + "getFactor(i).potential.size1():\t" + toString(getFactor(i).potential.size1()) + "\n");
      }
      if (nbCount > 1) {
	unsigned nb = nbs[1];
	if (getFactor(i).potential.size2() != getVariable(nb).dimension )
	  errorAbort("Inconsistent graph. \nFactor potential dimensions does not match dimension of neighboring variable node (both defined below):\n\n" + factorInfoStr(i)  + "\n" + variableInfoStr(i) + "getFactor(i).potential.size2():\t" + toString(getFactor(i).potential.size2()) + "\n" + "getVariable(nb).dimension:\t" + toString(getVariable(nb).dimension) + "\n" );
      }
      if (nbCount > 2)
	  errorAbort("Inconsistent graph. \nFactor with more than two neighbors currently not supported:\n" + factorInfoStr(i) );
    }
  }


  xnumber_t DFG::calcNormConst2(stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages) const 
  {
    return calcNormConst(0, stateMasks[0], inMessages[0]);
  }


  xnumber_t DFG::calcNormConst2(stateMaskVec_t const & stateMasks) const 
  {
    return calcNormConst(0, stateMasks[0], inMessages_[0]);
  }


  void DFG::runSumProduct(stateMaskVec_t const & stateMasks)
  {
    if (inMessages_.size() == 0) 
      initMessages();
    
    runSumProduct(stateMasks, inMessages_, outMessages_);
  }


  void DFG::runSumProduct(stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages) const
  {
    unsigned root = 0;
    runSumProductInwardsRec(root, root, stateMasks, inMessages, outMessages);
    runSumProductOutwardsRec(root, root, stateMasks, inMessages, outMessages);
  }


  void DFG::calcSumProductMessage(unsigned current, unsigned receiver, stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages) const
  {
    if (nodes[current].isFactor)
      calcSumProductMessageFactor(current, receiver, inMessages, outMessages);
    else
      calcSumProductMessageVariable(current, receiver, stateMasks, inMessages, outMessages);
  };


  // send requests from the root out, returns messages from the leaves
  // inwards. Note that messages are passed implicitly through the
  // tables of outMessages (and pointers of inMessages)
  void DFG::runSumProductInwardsRec(unsigned current, unsigned sender, stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages) const
  {
    vector<unsigned> const & nbs = neighbors[current];
    // recursively call all nodes
    for (unsigned i = 0; i < nbs.size(); i++) {
      unsigned nb = nbs[i];
      if (nb != sender) 
	runSumProductInwardsRec(nb, current, stateMasks, inMessages, outMessages);
    }

    // calc outMessage and store in outMessages -- inMessages provides links to outMessages, which provides the message passing.
    if (current == sender) // this is root node of recursion
      return;
    calcSumProductMessage(current, sender, stateMasks, inMessages, outMessages); //  receiver = sender
  }


  // send messages from the root outwards to the leaves. Precondition: DFG::runSumProductInwardsRec must have been run on inMessages and outMessages.
  void DFG::runSumProductOutwardsRec(unsigned current, unsigned sender, stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages) const
  {
    vector<unsigned> const & nbs = neighbors[current];
    // recursively call all nodes
    for (unsigned i = 0; i < nbs.size(); i++) {
      unsigned nb = nbs[i];
      if (nb != sender) {
	calcSumProductMessage(current, nb, stateMasks, inMessages, outMessages); //  receiver = nb
	runSumProductOutwardsRec(nb, current, stateMasks, inMessages, outMessages);
      }
    }
  }



  xnumber_t DFG::runMaxSum(stateMaskVec_t const & stateMasks, vector<unsigned> & maxVariables)
  {
    // init data structures
    if (inMessages_.size() == 0)
      initMessages();
    if (maxNeighborStates_.size() == 0)
      initMaxNeighbourStates();
    
    return runMaxSum(stateMasks, maxVariables, inMessages_, outMessages_, maxNeighborStates_);
  }


  xnumber_t DFG::runMaxSum(stateMaskVec_t const & stateMasks, vector<unsigned> & maxVariables, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages, vector<vector<vector<unsigned> > > & maxNeighborStates) const
  {
    unsigned root = 0;
    runMaxSumInwardsRec(root, root, stateMasks, inMessages, outMessages, maxNeighborStates);

    // find max at root node
    unsigned dim = nodes[root].dimension;
    xvector_t v(dim);
    calcSumProductMessageVariable(root, root, stateMasks[root], inMessages[root], v);  // sumProduct and maxSum perform the same calculations for variables
    unsigned maxState = ublas::index_norm_inf(v);
    xnumber_t maxVal = v[maxState];

    //  backtrack
    if (maxVariables.size() == 0)
      initMaxVariables(maxVariables);
    backtrackMaxSumOutwardsRec(maxVariables, root, root, maxState, maxNeighborStates);

    return maxVal;
  }


  void DFG::runMaxSumInwardsRec(unsigned current, unsigned sender, stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages, vector<vector<vector<unsigned> > > & maxNeighborStates) const
  {
    vector<unsigned> const & nbs = neighbors[current];
    // recursively call all nodes
    for (unsigned i = 0; i < nbs.size(); i++) {
      unsigned nb = nbs[i];
      if (nb != sender) 
	runMaxSumInwardsRec(nb, current, stateMasks, inMessages, outMessages, maxNeighborStates);
    }

    // calc outMessage and store in outMessages -- inMessages provides links to outMessages, which provides the message passing.
    if (current == sender) // this is root node of recursion
      return;
    calcMaxSumMessage(current, sender, stateMasks, inMessages, outMessages, maxNeighborStates); //  receiver = sender
  }


  void DFG::calcMaxSumMessage(unsigned current, unsigned receiver, stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages, vector<vector<vector<unsigned> > > & maxNeighborStates) const
  {
    if (nodes[current].isFactor)
      calcMaxSumMessageFactor(current, receiver, inMessages, outMessages, maxNeighborStates);
    else
      calcSumProductMessageVariable(current, receiver, stateMasks, inMessages, outMessages); // calc for variables same as in sum product algorithm
  };


  void DFG::calcMaxSumMessageFactor(unsigned current, unsigned receiver, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages, vector<vector<vector<unsigned> > > & maxNeighborStates) const
  {
    vector<unsigned> const & nbs = neighbors[current];
    xvector_t & outMes = outMessages[current][ getIndex( nbs, receiver) ];  // identify message
    vector< xvector_t const *> const & inMes( inMessages[current] );
    vector<vector<unsigned> > & maxNBStates = maxNeighborStates[ convNodeToFac(current) ];

    calcMaxSumMessageFactor(current, receiver, inMes, outMes, maxNBStates);
  }


  void DFG::calcMaxSumMessageFactor(unsigned current, unsigned receiver, vector<xvector_t const *> const & inMes, xvector_t & outMes, vector<vector<unsigned> > & maxNBStates) const
  {
    vector<unsigned> const & nbs = neighbors[current];
    DFGNode const & nd = nodes[current];
    xmatrix_t const & pot = nd.potential;

    // one neighbor 
    if (nd.dimension == 1) {
      for (unsigned i = 0; i < pot.size2(); i++)
	outMes[i] = pot(0, i);
      return;
    }

    // two neighbors
    if (nd.dimension == 2) {
      if (nbs[0] == receiver) {
	unsigned nbIdx = 1;
	for (unsigned i = 0; i < pot.size1(); i++) {
	  xvector_t v = elemProd<xvector_t>(*inMes[nbIdx], ublas::matrix_row<xmatrix_t const>(pot, i));
	  unsigned maxIdx = ublas::index_norm_inf(v);
	  maxNBStates[nbIdx][i] = maxIdx;
	  outMes[i] = v[maxIdx];
	}
      }
      else { // nbs[1] == receiver
	unsigned nbIdx = 0;
	for (unsigned i = 0; i < pot.size2(); i++) {
	  xvector_t v = elemProd<xvector_t>(*inMes[nbIdx], ublas::matrix_column<xmatrix_t const>(pot, i) );
	  unsigned maxIdx = ublas::index_norm_inf(v);
	  maxNBStates[nbIdx][i] = maxIdx;
	  outMes[i] = v[maxIdx];
	}
      }
      return;
    }
    
    // more than two neighbors -- should not happen
    errorAbort("calcMaxSumMessageFactor: Error, factor with more than two neighbors");
  }


  void DFG::backtrackMaxSumOutwardsRec(vector<unsigned> & maxVariables, unsigned current, unsigned sender, unsigned maxState, vector<vector<vector<unsigned> > > & maxNeighborStates) const
  {
    // store variable max state
    if (not nodes[current].isFactor)
      maxVariables[ convNodeToVar(current) ] = maxState;

    vector<unsigned> const & nbs = neighbors[current];
    // recursively call all nodes
    for (unsigned i = 0; i < nbs.size(); i++) {
      unsigned nb = nbs[i];
      if (nb != sender) {
	unsigned nbMaxState;
	if (nodes[current].isFactor)
	  nbMaxState = maxNeighborStates[ convNodeToFac(current) ][i][maxState];
	else
	  nbMaxState = maxState; 
	backtrackMaxSumOutwardsRec(maxVariables, nb, current, nbMaxState, maxNeighborStates); //  receiver = nb
      }
    }
  }


  xnumber_t DFG::calcNormConst(stateMaskVec_t const & stateMasks)
  {
    if (inMessages_.size() == 0)
      initMessages();

    return calcNormConst(stateMasks, inMessages_, outMessages_);
  }


  xnumber_t DFG::calcNormConst(stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages) const
  {
    unsigned const root = 0;
    runSumProductInwardsRec(root, root, stateMasks, inMessages, outMessages);
    return calcNormConst(root, stateMasks[root], inMessages[root]);
  }


  xnumber_t DFG::calcNormConst(unsigned varId, stateMask_t const * stateMask, vector<xvector_t const *> const & inMes) const
  {
    assert( varId < variables.size() );
    unsigned dim = nodes[ variables[varId] ].dimension;
    xvector_t v(dim);
    calcSumProductMessageVariable(varId, varId, stateMask, inMes, v);
    return ublas::sum(v);
  }


  void DFG::calcVariableMarginals(vector<xvector_t> & variableMarginals, stateMaskVec_t const & stateMasks)
  {
    calcVariableMarginals(variableMarginals, stateMasks, inMessages_);
  }


  void DFG::calcVariableMarginals(vector<xvector_t> & variableMarginals, stateMaskVec_t const & stateMasks, vector<vector<xvector_t const *> > & inMessages) const
  {
    if (variableMarginals.size() == 0)
      initVariableMarginals(variableMarginals);
    xnumber_t const Z = calcNormConst(0, stateMasks[0], inMessages[0]);
    for (unsigned i = 0; i < variables.size(); i++) {
      calcSumProductMessageVariable(i, i, stateMasks[i], inMessages[i], variableMarginals[i]);
      variableMarginals[i] *= 1/ Z;
    }
  }


  void DFG::calcFactorMarginals(vector<xmatrix_t> & factorMarginals)
  {
    calcFactorMarginals(factorMarginals, inMessages_);
  }


  void DFG::calcFactorMarginals(vector<xmatrix_t> & factorMarginals, vector<vector<xvector_t const *> > & inMessages) const
  {
    if (factorMarginals.size() == 0)
      initFactorMarginals(factorMarginals);

    for (unsigned facId = 0; facId < factors.size(); facId++) {
      unsigned ndId = convFacToNode(facId);
      vector<xvector_t const *> const & inMes = inMessages[ndId];
      xmatrix_t & m = factorMarginals[facId];
      m = nodes[ndId].potential;  // set facMar = potential
      if (nodes[ndId].dimension == 1)
	for (unsigned i = 0; i < m.size2(); i++)
	  m(0, i) *= (*inMes[0])[i];
      else if (nodes[ndId].dimension == 2)
	for (unsigned i = 0; i < m.size1(); i++)
	  for (unsigned j = 0; j < m.size2(); j++)
	    m(i, j) *= (*inMes[0])[i] * (*inMes[1])[j];
      else // should not happen
	errorAbort("Factor with more than two neighbors. Aborts.");
       
      // normalize
    }

    xnumber_t const Z = sumMatrix(factorMarginals[0]);
    for (unsigned facId = 0; facId < factors.size(); facId++)
      factorMarginals[facId] *= 1/ Z;
  }


  // init data structures given in DFG.
  void DFG::initMessages()
  {
    initMessages(inMessages_, outMessages_);
  }


  void DFG::initMaxNeighbourStates()
  {
    initMaxNeighbourStates(maxNeighborStates_);
  }


  void DFG::initMessages(vector<vector<xvector_t const *> > & inMessages, vector<vector<xvector_t> > & outMessages) const
  {
    // outMessages
    // outMessages[i][j] is a the outmessage for the j'th neighbor of node i. It's dimension equals the dimension of the linking variable node.
    outMessages.clear();
    outMessages.resize( nodes.size() );

    for (unsigned i = 0; i < nodes.size(); i++) {
      DFGNode const & nd = nodes[i];
      if (nd.isFactor) {
	for (unsigned j = 0; j < neighbors[i].size(); j++) {
	  unsigned const dim = nodes[ neighbors[i][j] ].dimension; // all neighbors are variables
	  outMessages[i].push_back(xvector_t(dim, 0) ); 
	}
      }
      else {  // is variable
	unsigned const dim = nodes[i].dimension; 
	for (unsigned j = 0; j < neighbors[i].size(); j++)
	  outMessages[i].push_back(xvector_t(dim , 0) ); 
      }
    }

    // inMessages
    // inMessages[i][j] is the inMessage from the j'th neighbor of node i. It is a pointer to the corresponding outMessage. (The dimension of the vector pointed to equals the linking variable node).
    inMessages.clear();
    inMessages.resize( nodes.size() );
    for (unsigned i = 0; i < nodes.size(); i++)
      for (unsigned j = 0; j < neighbors[i].size(); j++) {
	unsigned nb = neighbors[i][j];
	unsigned k  = getIndex(neighbors[nb], i);
	inMessages[i].push_back(& outMessages[nb][k]);
      }
  }
  

  void DFG::initVariableMarginals(vector<xvector_t> & variableMarginals) const
  {
    initGenericVariableMarginals(variableMarginals, *this);
//
//    variableMarginals.clear();
//    variableMarginals.resize( variables.size() );
//    for (unsigned i = 0; i < variables.size(); i++) {
//      unsigned dim = nodes[ variables[i] ].dimension;
//      variableMarginals[i].resize(dim);
//    }
//    reset(variableMarginals);
  }
  

  void DFG::initFactorMarginals(vector<xmatrix_t> & factorMarginals) const
  {
    initGenericFactorMarginals(factorMarginals, *this);
//    factorMarginals.clear();
//    factorMarginals.resize( factors.size() );
//    for (unsigned i = 0; i < factors.size(); i++) {
//      unsigned size1 = nodes[ factors[i] ].potential.size1();
//      unsigned size2 = nodes[ factors[i] ].potential.size2();
//      factorMarginals[i].resize(size1, size2);
//    }
//    reset(factorMarginals);
  }

  
  // get max neighbor dimension
  unsigned DFG::maxNeighborDimension(vector<unsigned> const & nbs) const
  {
    unsigned maxDim = 0;
    BOOST_FOREACH(unsigned idx, nbs) {
      unsigned dim = nodes[idx].dimension;
      if ( dim > maxDim) 
	maxDim = dim;
    }
    return maxDim;
  }


  // Sets up the data structure needed for maxSum. Indexing:
  // maxNeighborStates[i][j] returns a vector of unsigned (states)
  // corresponding to the j'th neigbor of the i'th factor.
  void DFG::initMaxNeighbourStates(vector<vector<vector<unsigned> > > & maxNeighborStates) const
  {
    maxNeighborStates.clear();

    maxNeighborStates.resize( factors.size() );
    for (unsigned i = 0; i < factors.size(); i++) {
      unsigned const dim = nodes[ factors[i] ].dimension;
      maxNeighborStates[i].resize(dim);
      unsigned maxVarDim = maxNeighborDimension( getFactorNeighbors(i) );
      for (unsigned j = 0; j < dim; j++)
	maxNeighborStates[i][j].resize(maxVarDim);
    }
  }
  

  void DFG::initMaxVariables(vector<unsigned> & maxVariables) const
  {
    maxVariables.clear();
    maxVariables.resize( variables.size() );
  }

  vector<xnumber_t> calcNormConsMultObs(stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    vector<xnumber_t> result( stateMask2DVec.size() );
    calcNormConsMultObs(result, stateMask2DVec, dfg);
    return result;
  }

  void DFG::init(vector<unsigned> const & varDimensions, vector<xmatrix_t> const & facPotentials, vector<vector<unsigned> > const & facNeighbors)
  {
    // reserve memory
    nodes.reserve( varDimensions.size() + facPotentials.size() ); 
    neighbors.reserve( varDimensions.size() + facPotentials.size() ); 
    variables.reserve( varDimensions.size() );
    factors.reserve( facPotentials.size() );

    // define variable nodes
    unsigned idx = 0;
    BOOST_FOREACH(unsigned dim, varDimensions) {
      nodes.push_back( DFGNode(dim) );
      variables.push_back(idx);
      idx++;
    }
      
    // define factor nodes
    BOOST_FOREACH(xmatrix_t const & pot, facPotentials) {
      nodes.push_back( DFGNode( pot ) );
      factors.push_back(idx);
      idx++;
    }

    neighbors.resize( variables.size() ); // placeholder for var neighbors
    neighbors.insert(neighbors.end(), facNeighbors.begin(), facNeighbors.end());  // all facNeighbors refer to variables, which have not changed indices
    addTwoWayLinks(neighbors); // now add the var neighbors

    consistencyCheck();
  }



  void calcNormConsMultObs(vector<xnumber_t> & result, stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    for (unsigned i = 0; i < stateMask2DVec.size(); i++)
      result[i] = dfg.calcNormConst(stateMask2DVec[i]);
  }


  // Calculates the accumulated variable marginals over all observation vectors. 
  vector<vector_t> calcVarAccMarMultObs(stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    vector<vector_t> accVariableMarginals;
    initAccVariableMarginals(accVariableMarginals, dfg);
    calcVarAccMarMultObs(accVariableMarginals, stateMask2DVec, dfg);
    return accVariableMarginals;
  }


  void calcVarAccMarMultObs(vector<vector_t> & result, stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    unsigned varCount = dfg.variables.size();
    assert(result.size() == varCount);

    vector<xvector_t> tmpVarMar;
    dfg.initVariableMarginals(tmpVarMar);
    for (unsigned i = 0; i < stateMask2DVec.size(); i++) {
      dfg.runSumProduct(stateMask2DVec[i]);
      dfg.calcVariableMarginals(tmpVarMar, stateMask2DVec[i]);
      for (unsigned j = 0; j < varCount; j++)
	result[j] += toNumber( tmpVarMar[j] ); // need to convert xnumber_t to number_t
    }
  }


  // Calculates the accumulated factor marginals over all observation vectors. 
  vector<matrix_t> calcFacAccMarMultObs(stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    vector<matrix_t> accFactorMarginals;
    initAccFactorMarginals(accFactorMarginals, dfg);
    calcFacAccMarMultObs(accFactorMarginals, stateMask2DVec, dfg);
    return accFactorMarginals;
  }


  void calcFacAccMarMultObs(vector<matrix_t> & result, stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    unsigned facCount = dfg.factors.size();
    assert(result.size() == facCount);

    vector<xmatrix_t> tmpFacMar;
    dfg.initFactorMarginals(tmpFacMar);
    for (unsigned i = 0; i < stateMask2DVec.size(); i++) {
      dfg.runSumProduct(stateMask2DVec[i]);
      dfg.calcFactorMarginals(tmpFacMar);
      for (unsigned j = 0; j < facCount; j++)
	result[j] += toNumber( tmpFacMar[j] );
    }
  }


  void calcVarAndFacAccMarMultObs(vector<vector_t> & varResult, vector<matrix_t> & facResult, stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    unsigned varCount = dfg.variables.size();
    assert(varResult.size() == varCount);

    unsigned facCount = dfg.factors.size();
    assert(facResult.size() == facCount);

    vector<xvector_t> tmpVarMar;
    dfg.initVariableMarginals(tmpVarMar);
    vector<xmatrix_t> tmpFacMar;
    dfg.initFactorMarginals(tmpFacMar);
    for (unsigned i = 0; i < stateMask2DVec.size(); i++) {
      dfg.runSumProduct(stateMask2DVec[i]);
      dfg.calcVariableMarginals(tmpVarMar, stateMask2DVec[i]);
      for (unsigned j = 0; j < varCount; j++)
	varResult[j] += toNumber( tmpVarMar[j] );
      dfg.calcFactorMarginals(tmpFacMar);
      for (unsigned j = 0; j < facCount; j++)
	facResult[j] += toNumber( tmpFacMar[j] );
    }
  }


  void calcMaxProbStatesMultObs(vector<xnumber_t> & maxProbResult, vector<vector<unsigned> > & maxVarResult, stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    unsigned long stateMaskVecCount = stateMask2DVec.size();
    unsigned varCount = dfg.variables.size();

    // adjust result references if necessary
    if (maxProbResult.size() != stateMask2DVec.size() )
      maxProbResult.resize(stateMaskVecCount);

    if (maxVarResult.size() != stateMaskVecCount)
      maxVarResult.resize(stateMaskVecCount);
    assert(varCount > 0);
    if (maxVarResult[0].size() != varCount)
      for (unsigned i = 0; i < stateMaskVecCount; i++)
	maxVarResult[i].resize(varCount);
    
    // calculations
    for (unsigned i = 0; i < stateMaskVecCount; i++)
      maxProbResult[i] = dfg.runMaxSum(stateMask2DVec[i], maxVarResult[i]);
  }


  pair<vector<xnumber_t>, vector<vector<unsigned> > > calcMaxProbStatesMultObs(stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    long stateMaskVecCount = stateMask2DVec.size();
    unsigned varCount = dfg.variables.size();
    vector<xnumber_t> maxProbResult(stateMaskVecCount);
    vector<vector<unsigned> > maxVarResult(stateMaskVecCount, vector<unsigned>(varCount) );

    calcMaxProbStatesMultObs(maxProbResult, maxVarResult, stateMask2DVec, dfg);
    return make_pair(maxProbResult, maxVarResult);
  }

  // tmpFacMar is used as workspace. accFacMar must be of correct size (use dfg.initFactorMarginals(tmpFacMar) )
  void calcFacAccMarAndNormConst(vector<matrix_t> & accFacMar, vector<xmatrix_t> & tmpFacMar, xnumber_t & normConst, stateMaskVec_t const & stateMaskVec, DFG & dfg)
  {
    dfg.runSumProduct(stateMaskVec);
    dfg.calcFactorMarginals(tmpFacMar);
    for (unsigned j = 0; j < dfg.factors.size(); j++)
      accFacMar[j] += toNumber( tmpFacMar[j] );
    normConst = dfg.calcNormConst2(stateMaskVec);
  }


  void calcFacAccMarAndNormConstMultObs(vector<matrix_t> & accFacMar, xvector_t & normConstVec, stateMask2DVec_t const & stateMask2DVec, DFG & dfg)
  {
    // check data structures
    long unsigned stateMaskVecCount = stateMask2DVec.size();
    assert( accFacMar.size() == dfg.factors.size() );
    if( normConstVec.size() != stateMaskVecCount )
      normConstVec.resize( stateMaskVecCount );

    // calculations
    vector<xmatrix_t> tmpFacMar;
    dfg.initFactorMarginals(tmpFacMar);
    for (unsigned i = 0; i < stateMaskVecCount; i++) {
      calcFacAccMarAndNormConst(accFacMar, tmpFacMar, normConstVec[i], stateMask2DVec[i], dfg);
    }
  }

  // helper functions
  void initAccVariableMarginals(vector<vector_t> & variableMarginals, DFG const & dfg)
  {
    initGenericVariableMarginals(variableMarginals, dfg);
  }
    
  void initAccFactorMarginals(vector<matrix_t> & factorMarginals, DFG const & dfg)
  {
    initGenericFactorMarginals(factorMarginals, dfg);
  }


} // namespace phy

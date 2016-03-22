/*
 * Helper.h
 *
 *  Created on: Mar 6, 2016
 *      Author: Aniruddha Gokhale
 */

#ifndef CS6381_CHORD_P2P_HELPER_H_
#define CS6381_CHORD_P2P_HELPER_H_

#include <string>
#include <vector>
#include <set>
using namespace std;

#include "inet/networklayer/common/L3AddressResolver.h"

class Helper {
public:
    // this is a data structure to maintain the mapping between
    // a chord node Id and its IP address
    struct Id2AddrEntry {
        int nodeID;             // chord node ID
        string moduleName;      // module name
        inet::L3Address addr;   // L3Address
    };
    typedef vector<Id2AddrEntry> Id2AddrMap;

    typedef vector<int> IntVector;
    typedef set<int> IntSet;

    Helper (int m, int numChordNodes, int numLookupKeys, int numItersPerLookup)
        : m_ (m),
          numChordNodes_ (numChordNodes),
          numLookupKeys_ (numLookupKeys),
          numItersPerLookup_ (numItersPerLookup),
          chordNodeList_ (),
          map_ ()
    {
    }

    ~Helper (void) {}

    // initialize the helper class. In particular, initialize a randomly created
    // list of chord nodes
    void init_chord_node_list (void);

    // the bits used to encode the total key space
    int num_bits (void) { return this->m_;}

    // the total num of chord nodes
    int num_chord_nodes (void) {return this->numChordNodes_; }

    // the total num of chord nodes
    int num_iters_per_lookup (void) {return this->numItersPerLookup_; }

    // create a randomly generated set of lookup keys for the client to use
    void gen_lookup_keys (IntVector &iv);

    // return the generated list of chord node IDs in sorted order
    void chord_node_list (IntVector &iv);

    // register_node. Every chord node will register with this helper database
    // when it has initialized itself and has its IP address
    void register_node (int nodeID, const inet::L3Address &addr);

    // lookup a node based on its id and return its addr
    inet::L3Address lookup_node (int nodeID);

    // here we define some helper functions that can be used by our applications
    void tokenize_and_sort (const string &s, IntVector &iv);

private:
    int m_;                 // num of bits
    int numChordNodes_;     // num of chord nodes
    int numLookupKeys_;     // num of lookup keys to generate
    int numItersPerLookup_; // num of iterations per lookup request

    IntVector chordNodeList_;  // list of chord nodes generated
    Id2AddrMap  map_;       // database of node ID and IP address mapping
};

// a global variable used by all other modules
extern Helper *helper;

#endif /* CS6381_CHORD_P2P_HELPER_H_ */

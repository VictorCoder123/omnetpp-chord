/*
 * Helper.cc
 *
 *  Created on: Mar 6, 2016
 *      Author: Aniruddha Gokhale
 */

#include <algorithm>
#include <random>
using namespace std;

#include <omnetpp.h>

#include "Helper.h"     // header file

// we shall have the coordinator create us and initialize us
Helper *helper = NULL;

// initialize the helper class. In particular, initialize a randomly created
// list of chord nodes
void Helper::init_chord_node_list (void)
{
    // our goal is to generate a bunch of node IDs that
    // lie in the range 0 to 2^m - 1 without any repetitions
    int key_space = (1 << this->m_);
    Helper::IntSet  is;
    std::mt19937 generator (key_space); // mersenne_twister_engine random num generator
    for (int i = 1; i <= this->numChordNodes_; ++i) {
        // we need unique node ids. It is possible that a random num gen
        // will produce same num and we don't want to add the same num to the
        // set. So we keep inserting a new number until our length is the same
        // as what our current notion of size of the array ought to be
        while (is.size () != i) {
            int id = generator () % key_space;   // generates a number between 0 .. key-space-1
            is.insert (id);
        }
    }

    // now copy over all elements of the set to our vector
    for (Helper::IntSet::iterator it = is.begin (); it != is.end (); ++it) {
        EV << "+++++ generated node id = " << (*it) << endl;
        this->chordNodeList_.push_back (*it);
    }
}

// create a randomly generated set of lookup keys for the client to use
void Helper::gen_lookup_keys (Helper::IntVector &iv)
{
    // we will add logic later
    // return string ("2 0 6 1 12 15 10 8 4 3 13");
    int key_space = (1 << this->m_);
    std::mt19937 generator (key_space); // mersenne_twister_engine random num generator
    for (int i = 0; i < this->numLookupKeys_; ++i) {
        iv.push_back (generator () % key_space);
    }
    for (Helper::IntVector::iterator it = iv.begin (); it != iv.end (); ++it) {
        EV << "+++++ generated lookup key = " << (*it) << endl;
    }
}

// return the generated list of nodes
void Helper::chord_node_list (Helper::IntVector &iv)
{
    iv = this->chordNodeList_;
    return;
}

// lookup a node based on its id and return its addr
inet::L3Address Helper::lookup_node (int nodeID)
{
    EV << "==== Helper::lookup_node: nodeID = " << nodeID
       << " =====" << endl;

    // search our map
    for (Helper::Id2AddrMap::iterator it = this->map_.begin ();
            it != this->map_.end (); ++it) {
        if ((*it).nodeID == nodeID)
            return (*it).addr;
    }

    // if not found, technically we should throw an exception
    inet::L3Address addr;
    return addr;
}

// register_node
void Helper::register_node (int nodeID, const inet::L3Address &addr)
{
    EV << "==== Helper::register_node: nodeID = " << nodeID
       << ", addr = " << addr.str () << " =====" << endl;
    Helper::Id2AddrEntry entry;
    entry.nodeID = nodeID;
    entry.addr = addr;
    this->map_.push_back (entry);
}

void Helper::tokenize_and_sort (const string &s, IntVector &iv)
{
    // given a string, we tokenize it into a vector of ints
    // and then sort the vector
    EV << "tokenize_and_sort the string: " << s << endl;

    size_t start = 0;
    size_t found = s.find_first_of (" ");
    while (found!=std::string::npos) {
        iv.push_back (stol (s.substr (start, (found-start))));
        start = found+1;
        found=s.find_first_of(" ", start);
    }
    // remainder string is the last number
    iv.push_back (stol (s.substr (start)));

    EV << "---- Before sort ---" << endl;
    for (IntVector::iterator it=iv.begin(); it!=iv.end(); ++it)
        EV << ' ' << (*it);
    EV << endl;

    sort (iv.begin (), iv.end ());

    EV << "---- After sort ---" << endl;
    for (IntVector::iterator it=iv.begin(); it!=iv.end(); ++it)
        EV << ' ' << *it;
    EV << endl;
}


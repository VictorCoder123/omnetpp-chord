/*
 * Coordinator.h
 *
 *  Created on: March 3, 2016
 *      Author: Aniruddha Gokhale
 */

#ifndef CS6381_CHORD_P2P_Coordinator_H_
#define CS6381_CHORD_P2P_Coordinator_H_

#include <vector>
#include <string>
#include <map>
using namespace std;

#include <omnetpp.h>
using namespace omnetpp;

#include "inet/common/INETDefs.h"  // this contains imp definitions from the INET

/**
 * This is our Coordinator
 *
 *
 */

class Coordinator : public cSimpleModule, public cListener
{
public:
    typedef vector<simtime_t> TimeVector;
    typedef map <string, TimeVector> RTTMap;

    /**
     *  constructor
     */
    Coordinator (void);

    /**
     * destructor
     */
    virtual ~Coordinator (void);

protected:
    /**
     * Initialization.
     */
    virtual void initialize (int stage) override;

    /**
     * define how many initialization stages are we going to need.
     */
    virtual int numInitStages (void) const override { return inet::NUM_INIT_STAGES+1; }

    /**
     * master event handler method.
     */
    virtual void handleMessage (cMessage *msg) override;

    // wrap up
    virtual void finish() override;

    // overriden methods of Listener class
    virtual void    receiveSignal (cComponent *source, simsignal_t signalID, const SimTime &t, cObject *details) override;

    virtual void finish(cComponent *component, simsignal_t id) override;

private:
    static simsignal_t sentLookupSignal;
    static simsignal_t rcvdRespSignal;

    // all params obtained from simulation files
    int m_;
    int numChordNodes_;
    int numClients_;
    int numLookupKeys_;
    int numItersPerLookup_;


    // internal variables
    int totalClientRequests_;      // number of clients in the system
    int requestsCompleted_;        // number of client requests completed so far
    RTTMap  map_;


};



#endif /* CS6381_CHORD_P2P_Coordinator_H_ */

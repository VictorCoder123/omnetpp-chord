/*
 * ChordNode.h
 *
 *  Created on: March 3, 2016
 *
 *      Author: Aniruddha Gokhale
 *      Class:  CS6381 - Distributed Systems Principles
 *      Institution: Vanderbilt University
 */

#ifndef _CS6381_CHORD_NODE_H_
#define _CS6381_CHORD_NODE_H_

#include <string>
#include <vector>
#include <map>
#include <stack>
using namespace std;

#include "inet/common/INETDefs.h"  // this contains imp definitions from the INET
#include "inet/transportlayer/contract/tcp/TCPSocket.h" // this is needed for sockets
#include "inet/transportlayer/contract/tcp/TCPSocketMap.h"  // this is needed to maintain multiple connected sockets from other peers

#include "Helper.h" // helper functions

class ChordNode : public cSimpleModule,
                  public inet::TCPSocket::CallbackInterface
{
  public:
    //* @@@ FILL IN @@@ */
    // data structure for the finger table
    // I am maintaining this mapping but if you feel you need more details, please
    // modify the data structure
    struct Fingertable {
        int fingerID;               // id of the i_th finger
        inet::TCPSocket    *socket; // socket connection to that finger
    };

    //* @@@ FILL IN @@@ */
    // You need a data structure declaration here: The purpose is mentioned below in the
    // comments
    //
    // The following data structure is going to be used to preserve the calling socket.
    // This is needed for the case when we cannot find the key with ourselves and so must
    // pass it on to the next node using the chord algo. When the reply gets relayed back,
    // we will need to preserve the state, i.e., the socket, so that we can relay the
    // response upstream using the saved socket pointer.
    //
    // Our assumption here is that the client is not multithreaded and hence can
    // participate in only one lookup at a time.

    // Hint: I think you will need to keep some map indexed by the sender id

    /**
     *  constructor
     */
    ChordNode (void);

    /**
     * destructor
     */
    virtual ~ChordNode (void);

  private:
    int myID_;               // our ID
    inet::L3Address localAddress_;    // our local address
    int localPort_;          // our local port we will listen on (from NED file)
    int finger_table_size_;  // length of our finger table (= m, supplied as param to coordinator)
    Helper::IntVector nodeList_;     // list of nodes passed from simulation from which we pick the fingers

    // maintain a finger table data structure here
    Fingertable *ft_;

    // additional parameters
    inet::TCPSocket    *socket_;   // our main listening socket
    inet::TCPSocketMap socketMap_; // map of sockets we maintain for connections we
                                   // make to our fingers or connections we have
                                   // received from our fingers or clients

    //* @@@ FILL IN @@@ */
    // the data structure you created to preserve state for relaying responses
    /* your data member declaration goes here */
    int callerID;

  protected:
    /**
     * Initialization. Should be redefined to perform or schedule a connect().
     */
    virtual void initialize (int stage);

    /**
     * define how many initialization stages are we going to need.
     */
    virtual int numInitStages (void) const { return inet::NUM_INIT_STAGES+2; }

    /**
     * For self-messages it invokes handleTimer(); messages arriving from TCP
     * will get dispatched to the socketXXX() functions.
     */
    virtual void handleMessage (cMessage *msg);

    /**
     * Records basic statistics: numSessions, packetsSent, packetsRcvd,
     * bytesSent, bytesRcvd. Redefine to record different or more statistics
     * at the end of the simulation.
     */
    virtual void finish();


    /** @name Utility functions */

    //@{

    /** Invoked from handleMessage(). Should be defined to handle self-messages. */
    virtual void handleTimer (cMessage *msg);

    /** When running under GUI, it displays the given string next to the icon */
    virtual void setStatusString (const char *s);
    //@}


    /** @name TCPSocket::CallbackInterface callback methods */

    //@{
    /** Does nothing but update statistics/status. Redefine to perform or schedule first sending. */
    virtual void socketEstablished(int connId, void *yourPtr);

    /**
     * Does nothing but update statistics/status. Redefine to perform or schedule next sending.
     * Beware: this function deletes the incoming message, which might not be what you want.
     */
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent);

    /** Since remote TCP closed, invokes close(). Redefine if you want to do something else. */
    virtual void socketPeerClosed(int connId, void *yourPtr);

    /** Does nothing but update statistics/status. Redefine if you want to do something else, such as opening a new connection. */
    virtual void socketClosed(int connId, void *yourPtr);

    /** Does nothing but update statistics/status. Redefine if you want to try reconnecting after a delay. */
    virtual void socketFailure(int connId, void *yourPtr, int code);

    /** Redefine to handle incoming TCPStatusInfo. */
    virtual void socketStatusArrived(int connId, void *yourPtr, inet::TCPStatusInfo *status) {delete status;}
    //@}

    /** @name Helper functions */

    //@{

    /** build the finger table for this node */
    void init_finger_table ();

    /** find successor node given some key id*/
    int successor (int id);

    /** serve the incoming lookup request */
    void serve_lookup (Lookup_Req *req, inet::TCPSocket *socket);

    /** relay the response up the chain */
    void relay_resp (Lookup_Resp *resp);

    /** Issues a connection command to a finger */
    virtual inet::TCPSocket *connect (int fingerID);
    //@}
};


#endif /* _CS6381_CHORD_NODE_H_ */

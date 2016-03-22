/*
 * Client.h
 *
 *  Created on: March 4, 2016
 *
 *      Author: Aniruddha Gokhale
 *      Class:  CS6381 - Distributed Systems Principles
 *      Institution: Vanderbilt University
 */

#ifndef _CS6381_CLIENT_H_
#define _CS6381_CLIENT_H_

#include <string>
#include <vector>
#include <memory>
using namespace std;

#include <omnetpp.h>

#include "inet/common/INETDefs.h"   // common definitions
#include "inet/transportlayer/contract/tcp/TCPSocket.h" // socket API
#include "inet/transportlayer/contract/tcp/TCPSocketMap.h" // this is needed to
                                                           // maintain multiple
                                                           // connected sockets
                                                           // from other peers 
#include "inet/applications/tcpapp/TCPAppBase.h"    // we derive from app base

#include "Helper.h" // helper functions

/**
 * This is our client that makes a lookup request on the node
 *
 *
 */

class Client : public cSimpleModule,
               public inet::TCPSocket::CallbackInterface
{
  public:

    /**
     *  constructor
     */
    Client (void);

    /**
     * destructor
     */
    virtual ~Client (void);

  private:

    // these are all the variables from the NED file
    string myID_;            // our ID
    int chordNodePort_;      // port number on which the chord node listens to
    int numItersPerLookup_;  // how many iterations per lookup
    Helper::IntVector nodeList_;     // list of chord nodes
    Helper::IntVector lookupKeys_;      // list of keys to lookup

    // these are the additional variables we need for the business logic
    inet::TCPSocket  *socket_;   // our socket to talk to the server

    // curr iteration number
    int currIter_;

    // index of the next lookup server
    int nextKeyIndex_;

    static simsignal_t sentLookupSignal;
    static simsignal_t rcvdRespSignal;

  protected:
    /**
     * Initialization. Should be redefined to perform or schedule a connect().
     */
    virtual void initialize (int stage);

    /**
     * define how many initialization stages are we going to need.
     */
    virtual int numInitStages (void) const { return inet::NUM_INIT_STAGES+3; }

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


    /** @name TCPSocket::CallbackInterface callback methods */

    //@{
    /** Invoked when socket is established */
    virtual void socketEstablished(int connId, void *yourPtr);

    /**
     * Invoked when data arrives at the socket. 
     */
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent);

    /** Invoked when remote entity closes connection */
    virtual void socketPeerClosed(int connId, void *yourPtr);

    /** Invoked when we actively close the connection and get notified of a
        successful connection close. */
    virtual void socketClosed(int connId, void *yourPtr);

    /** Invoked when something fails w.r.t the socket. */
    virtual void socketFailure(int connId, void *yourPtr, int code);

    /** Redefine to handle incoming TCPStatusInfo. */
    virtual void socketStatusArrived(int connId, void *yourPtr, inet::TCPStatusInfo *status) {delete status;}
    //@}

    /** @name Utility functions */

    //@{

    /** Invoked from handleMessage(). Should be defined to handle self-messages. */
    virtual void handleTimer (cMessage *msg);

    /** Issues a connection command */
    virtual void connect (int idx);

    /** Issues CLOSE command */
    virtual void close (void);

    /** Sends a request */
    virtual void sendRequest (void);

    /** When running under GUI, it displays the given string next to the icon */
    virtual void setStatusString (const char *s);
    //@}

};

#endif /* _CS6381_CLIENT_H_ */

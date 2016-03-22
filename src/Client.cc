/*
 * Client.cc
 *
 *  Created on: March 4, 2016
 *
 *      Author: Aniruddha Gokhale
 *      Class:  CS6381 - Distributed Systems Principles
 *      Institution: Vanderbilt University
 */

#include <random>
using namespace std;

#include "ChordP2PMsg_m.h" // generated header from the message file
#include "Client.h"     // our header

#include "inet/common/INETEndians.h"  // for host/network byte ordering
#include "inet/networklayer/common/L3AddressResolver.h" // address resolution
using namespace inet;

#include "Helper.h"

// register module with Omnet++
Define_Module(Client);

simsignal_t Client::sentLookupSignal = registerSignal("sentLookupTS");
simsignal_t Client::rcvdRespSignal = registerSignal("rcvdRespTS");

// constructor and destructor
Client::Client (void)
    : cSimpleModule (),
      myID_ (),
      chordNodePort_ (10000),
      numItersPerLookup_ (1),
      nodeList_ (),
      lookupKeys_ (),
      socket_ (nullptr),
      currIter_ (0),
      nextKeyIndex_ (0)
{
    // nothing
}

Client::~Client()
{
    // nothing
}

// the initialize method invoked by the simulator during initialization stages
void Client::initialize (int stage)
{
    cSimpleModule::initialize (stage); // initialize our base class per stage

    if (stage != inet::NUM_INIT_STAGES+2)
        return;

    // obtain the values of parameters we have specified in NED file and ini
    // file 
    this->myID_ = this->par ("myID").stringValue ();
    // since there could be multiple clients, we are going to modify our ID
    // to be a hash of the default value plus few other things
    this->myID_ += std::to_string (this->getId ())
                + std::to_string (this->getParentModule()->getId ());

    this->chordNodePort_ = this->par ("chordNodePort");

    // retrieve various parameters from the helper
    this->numItersPerLookup_ = helper->num_iters_per_lookup ();
    helper->chord_node_list (this->nodeList_);
    helper->gen_lookup_keys (this->lookupKeys_);

    EV << "=== Client::initialize"
       << "\tmyID_ = " << this->myID_ << endl
       << "\tChord Node Port = " << this->chordNodePort_ << endl
       << "\tand signal IDs = " << Client::sentLookupSignal
       << " and " << Client::rcvdRespSignal << endl;
    
    // If there are lookups to be made, only then kick start things
    if (this->nextKeyIndex_ < this->lookupKeys_.size()) {
        // Note that this is a simulation. A simulation proceeds only if there are
        // events to process. To that end we now start a timer so that when it
        // kicks in, we make a connection to chord node to do a lookup
        cMessage *timer_msg = new cMessage ("connect", 0);  // kind is 0 for this msg
        if (!timer_msg) {
            throw cRuntimeError("Client::initialize -- no memory for timer message");
            return;
        }

        simtime_t time_at = simTime () + exponential (5);
        EV << "=== Client::initialize -- scheduling kickstart timer for lookup at "
                << time_at << endl;

        this->scheduleAt (time_at, timer_msg);
        setStatusString ("Timer Started");
    }
}

/** the all-purpose "handle message" method where we need to decide what action we would like to take */
void Client::handleMessage (cMessage *msg)
{
    EV << "=== Client::handleMessage " << this->myID_
       << " received handleMessage message" << endl;

    // check if this is a timer message generated the first time to wake us up
    // and start conversation with the server (or a subsequent timer)
    if (msg->isSelfMessage ()) // this is how we check it because we generated
                               // it for ourselves.
        this->handleTimer (msg);
    else {
        if (!this->socket_) {
            // socket was not initialized for some reason. Why?
            throw cRuntimeError("Client::handleMessage -- socket does not exist");
            return;
        } else {
            // the way we have programmed this logic is that any other event is
            // going to be a socket related event on the client side. So let our
            // socket handle it. The socket is guaranteed to be available because
            // it was created when the connection was established.
            this->socket_->processMessage(msg);
        }
    }
}

/** this method is provided to clean things up when the simulation stops and
    collect statistics 
*/
void Client::finish ()
{
    EV << "=== Client::finish called" << endl;

    // cleanup the socket
    delete this->socket_;
    this->socket_ = nullptr;
}

/** handle the timeout method */
void Client::handleTimer (cMessage *msg)
{
    // We use the "connection-per-session" model in which we make a connection per lookup
    // and keep it alive for all the iterations of the same lookup. We call this a session.
    //
    // Thus, kind == 0 => timer for making a connection
    //       kind == 1 => timer for next iteration on the same connection
    //       anything else is an exception
    if (msg->getKind() == 0) {
        EV << "@" << simTime () << " ,=== Client::handleTimer " << this->myID_
                << " being kickstarted to start a connection ===" << endl;
        setStatusString ("connecting");

        // here our aim is to send a lookup request. So first we must connect
        // to a chord node. Choose at random a chord node from the list of
        // chord nodes we have. Then first connect to that chord node.
        // However, make sure that we still have requests pending and only then
        // proceed with the connect request.
        //

        // make sure that we still have more lookups pending
        if (this->nextKeyIndex_ < this->lookupKeys_.size()) {
            // select a node at random from the list.
            std::mt19937 generator (1 << helper->num_bits()); // mersenne_twister_engine random num generator
            int nodeIndex = generator () % helper->num_chord_nodes();

            // connect to this node
            this->connect (nodeIndex);
        }
    } else if (msg->getKind() == 1) {
        EV << "@" << simTime () << " ,=== Client::handleTimer " << this->myID_
                << " being kickstarted to send next iteration ===" << endl;
        setStatusString ("next iteration");

        // here our aim is to send the same lookup request to the same chord node
        //
        this->sendRequest ();

    } else {
        throw cRuntimeError("Client::handleTimer -- unknown timer message");
    }

    // clean up.
    delete msg;
}


/*************************************************/
/** implement all the callback interface methods */
/*************************************************/

// this method is invoked when the connection is established
void Client::socketEstablished (int connID, void *role)
{
    EV << "=== Client::socketEstablished " << this->myID_
       << " received socketEstablished message on connID "
       << connID << " ===" << endl;

    this->setStatusString("ConnectionEstablished");

    // Now that the connection is established, we initiate the lookup request to the server
    this->sendRequest ();
}

/** handle incoming data. This is a response from the server */
void Client::socketDataArrived(int connID, void *, cPacket *msg, bool)
{
    // debugging output
    setStatusString ("Response");

    EV << "=== Client::socketDataArrived " << this->myID_
       << " received socketDataArrived message on connID " << connID
       << " of byte length = " << msg->getByteLength ()
       << " ===" << endl;

    this->emit (Client::rcvdRespSignal, simTime ());

    // incoming request ought to be Response packet.
    Lookup_Resp *resp = dynamic_cast<Lookup_Resp *> (msg);
    if (!resp) {
        throw cRuntimeError("Client::socketDataArrived -- not a CS_Resp packet type");
        return;
    }

    // print the details
    EV << "**** Client: Arriving packet: Lookup_Resp " << endl;
    EV << "\tLookup key = " << resp->getKey() << endl;
    EV << "\tsender = " << resp->getSender() << endl;
    EV << "\tNum of responders = " << resp->getResponderArraySize() << endl;
    for (int i=0; i < resp->getResponderArraySize(); ++i) {
        EV << "\t\tresponder[" << i << "] = " << resp->getResponder (i) << endl;
    }

    // cleanup the response message
    delete resp;

    // increment the iterations for this request
    this->currIter_++;

    // check if all iterations for this lookup request are done or not
    if (this->currIter_ == this->numItersPerLookup_) {
        // increment our lookup index
        this->nextKeyIndex_ ++;

        // reset iterations
        this->currIter_ = 0;

        // since we have received the response from the chord nodes for all the
        // iterations for this request, we actively close the connection to
        // the server. We will receive an ACK that the server has closed too in the
        // socketPeerclosed callback shown below.
        this->close ();

    } else {
        // still more iterations to go.
        // Just start a new timer and let the system handle the sending of the next request
        cMessage *timer_msg = new cMessage ("next_iter", 1);
        if (!timer_msg) {
            throw cRuntimeError("Client::socketDataArrived -- no memory for timer");
            return;
        }
        this->scheduleAt (simTime () + exponential (5), timer_msg);
    }
}

// we closed the socket and got notified
void Client::socketClosed (int connID, void *)
{
    EV << "=== Client::socketClosed " << this->myID_
       << " received socketClosed message on connID " << connID
       << " ===" << endl;

    setStatusString("socket closed");

    // we have received the ack for closing the connection. So we delete the
    // existing socket 
    delete this->socket_;
    this->socket_ = nullptr;


    // This logic is needed when more lookup keys remain.
    if (this->nextKeyIndex_ < this->lookupKeys_.size()) {
        // now we start a timer so that when it kicks in, we make a connection
        // to server 
        EV << "=== Client::socketClosed "
           << " still more requests to send. So start a new timer for new lookup request."
           << endl;
        
        cMessage *timer_msg = new cMessage ("connect", 0);
        if (!timer_msg) {
            throw cRuntimeError("Client::socketClosed -- no memory for timer");
            return;
        }
        this->scheduleAt (simTime () + exponential (5), timer_msg);
    }
}

// peer closed the socket
void Client::socketPeerClosed (int connID, void *)
{
    EV << "=== Client::socketPeerClosed " << this->myID_
       << " received socketPeerClosed message on connID "
       << connID << " ===" << endl;
    setStatusString("peer socket closed");

}

// something failed with sockets
void Client::socketFailure (int connID, void *, int code)
{
    EV << "=== Client::socketFailure " << this->myID_
       << " received socketFailure message on connID " << connID
       << " with failure code = " << code << " ===" << endl;

    setStatusString("connection broken");

    delete this->socket_;
    this->socket_ = nullptr;
}

/**********************************************************************/
/**           helper methods                                          */
/**********************************************************************/

// you may need to change the signature of this method to pass the chord node's addr as param
// this method establishes a connection to the server.
void Client::connect (int nodeIdx)
{
    // what we receive is an index into the node list
    EV << "=== Client::connect " << this->myID_
       << " connect to the chord node with ID"
       << this->nodeList_[nodeIdx] << " ======= " << endl;
    
    // create a new socket in the connecting role. Note that there should not
    // be an existing socket. If there is one, clean it up

    if (this->socket_)
        delete this->socket_;

    // allocate a new socket
    this->socket_ = new TCPSocket ();
    if (!this->socket_) {
        throw cRuntimeError("Client::connect -- no memory for socket");
        return;
    }

    // since we are going to use message passing to send actual
    // application-level messages to the other side, we use the OBJECT style
    // approach of communication.
    this->socket_->setDataTransferMode (TCP_TRANSFER_OBJECT);

    // don't forget to set the output gate for this socket. By doing this, we
    // link our application to the underlying TCP layer. I learned it the hard
    // way :-( 
    this->socket_->setOutputGate (gate ("tcpOut"));

    // do not forget to set ourselves as the callback on this socket so that
    // all network events can be handled by this socket. The first event we
    // should get after this is an indication that the connection is
    // established. We don't care about the second param here. 
    this->socket_->setCallbackObject (this, NULL);

    // Now issue a connect request. The L3AddressResolver class supplied by the
    // INET framework is a generic network layer address resolver and will take 
    // care of whether it is IPv4 or IPv6 or other protocols being used.  Here
    // we supply the server's address and port that we wish to connect to. 

    // Hint: see helper class' API to get the address of the node we are interested in

    inet::L3Address addr = helper->lookup_node (this->nodeList_[nodeIdx]);
    EV << "=== client::connect: address of node ID " << this->nodeList_[nodeIdx]
       << " = " << addr.str () << endl;

    this->socket_->connect (addr, this->chordNodePort_);

    // debugging
    EV << "+++ Client: " << this->myID_ << " created a new socket with "
       << "connection ID = " << this->socket_->getConnectionId ()
       << " +++" << endl;
}

// close the peer side. This is invoked by the client when it has received a
// response to what it requested.
void Client::close()
{
    EV << "=== Client::close -- " << this->myID_
       << " close the connection to the server " << endl;
    
    setStatusString("closing");
    this->socket_->close ();
}

// send a request to the other side
void Client::sendRequest (void)
{
    // populate a request packet with the details and send it
    // don't forget to start the round trip measurement timer

    Lookup_Req  *request = new Lookup_Req ();
    request->setKey (this->lookupKeys_[this->nextKeyIndex_]);
    request->setSender (this->myID_.c_str ());
    request->setByteLength (sizeof (int) + this->myID_.length() + 1);

    EV << "=== Client::sendRequest " << this->myID_
        << " making lookup request for key: "
        << request->getKey () << endl;

    // start the measurement of round trip delay
    this->emit (Client::sentLookupSignal, simTime ());

    // send to the chord node to whom we are connected
    this->socket_->send (request);

    return;
}

void Client::setStatusString(const char *s)
{
    if (hasGUI ()) {
        getDisplayString ().setTagArg ("t", 0, s);
        bubble (s);
    }
}

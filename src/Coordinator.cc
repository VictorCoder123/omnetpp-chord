/*
 * Coordinator.cc
 *
 *  Created on: March 3, 2016
 *      Author: Aniruddha Gokhale
 */

#include <fstream>
using namespace std;

#include "Coordinator.h"
#include "Helper.h"

// register module with Omnet++
Define_Module(Coordinator);

// retrieve the signal ids
simsignal_t Coordinator::sentLookupSignal = registerSignal("sentLookupTS");
simsignal_t Coordinator::rcvdRespSignal = registerSignal("rcvdRespTS");

Coordinator::Coordinator ()
    : cSimpleModule (),
      cListener (),
      m_ (0),
      numChordNodes_ (0),
      numClients_ (0),
      numLookupKeys_ (0),
      numItersPerLookup_ (0),
      totalClientRequests_ (0),
      requestsCompleted_ (0),
      map_ ()
{
}

Coordinator::~Coordinator ()
{
}

void Coordinator::initialize (int stage)
{
    cSimpleModule::initialize (stage); // initialize our base class per stage

    if (stage != inet::NUM_INIT_STAGES)
        // we do the initialization of our parameters after inet has taken care of everything
        return;

    // get our parameters
    this->m_ = this->par("m").longValue ();
    this->numChordNodes_ = this->par("numChordNodes").longValue ();
    this->numClients_ = this->par("numClients").longValue ();
    this->numLookupKeys_ = this->par("numLookupKeys").longValue ();
    this->numItersPerLookup_ = this->par("numItersPerLookup").longValue ();

    // compute the total number of requests that must be completed
    this->totalClientRequests_
        = this->numClients_ * this->numLookupKeys_ * this->numItersPerLookup_;

    // First subscribe to get notified on these signals.
    // from my understanding of how signals work, they can be passed up
    // from leaf nodes to the root. But I don't think they are
    // propagated to leaves of other branches unless we subscribe.
    // So the way to subscribe to signals emitted by client app, we
    // ask the root simulation environment's top-level system module
    // and let is subscribe to the signals of interest but pass
    // ourselves as the entity to be notified.

    getSimulation()->getSystemModule()->subscribe (sentLookupSignal, this);
    getSimulation()->getSystemModule()->subscribe (rcvdRespSignal, this);

    // now create the helper class and let it initialize itself
    helper = new Helper (this->m_, this->numChordNodes_,
                         this->numLookupKeys_, this->numItersPerLookup_);

    // given the parameters, create a chord node list
    helper->init_chord_node_list ();

    EV << "=== Coordinator::initialize ===\n"
            << "\tIn stage " << stage
            << ", Number of clients " << this->numClients_
            << ", and subscribing to " << getSignalName (sentLookupSignal)
            << " with ID = " << sentLookupSignal
            << " and " << getSignalName (rcvdRespSignal)
            << " with ID = " << rcvdRespSignal << endl;
}

void Coordinator::handleMessage (cMessage *msg)
{
    // because we are not generating any event for us, we should technically never be here.
    EV << "=== Coordinator::handleMessage -- WHAT AM I SUPPOSED TO DO HERE?" << endl;
}

void Coordinator::finish ()
{
    EV << "=== Coordinator::finish (cSimpleModule method) " << endl;

    // let's dump the RTT values in a comma separated file
    string filename = getSimulation()->getSystemModule()->getFullName();
    filename += ".csv";
    fstream fs;
    fs.open (filename, std::fstream::out);

    // Now let us iterate thru the map and print all the RTTs
    for (Coordinator::RTTMap::iterator it=this->map_.begin (); it !=this->map_.end (); ++it) {
        fs << it->first;
        Coordinator::TimeVector &tv = it->second;
        for (Coordinator::TimeVector::iterator tvit=tv.begin (); tvit != tv.end (); ++tvit) {
            fs << ", " << (*tvit);
        }
        fs << endl;
    }
    fs.close ();

}

void Coordinator::receiveSignal (cComponent *source, simsignal_t signalID, const SimTime &t, cObject *details)
{
    // This is the event we are interested in which will be emitted by the
    // sending of a request packet and receipt of a response

    // check if signal is send request or receive response
    if (signalID == Coordinator::sentLookupSignal) {
        EV << "=== Coordinator::receiveSignal (send)"
                << "\tSource comp name = " << source->getFullPath()
                << "\tSourc comp ID" << source->getId ()
                << "\tSignal ID = " << signalID
                << "\tSignal Name = " << getSignalName (signalID)
                << "\tSimTime = " << t
                << "\tobject details = " << (details? details->getFullName() : "Empty")
                << endl;

        // this signal is sent only once per request transmission by a client.
        // We use the client's fully qualified name as the "key" to store the value
        this->map_[source->getFullPath()].push_back (t);

    } else if (signalID == Coordinator::rcvdRespSignal) {
        // this signal is sent only once per response packet received by a client.
        // We use the client's fully qualified name as the "key" to retrieve the
        // packet transmission time and then subtract it from this value to give us
        // RTT and save it
        EV << "=== Coordinator::receiveSignal (resp)"
                << "\tSource comp name = " << source->getFullPath()
                << "\tSourc comp ID" << source->getId ()
                << "\tSignal ID = " << signalID
                << "\tSignal Name = " << getSignalName (signalID)
                << "\tSimTime = " << t
                << "\tobject details = " << (details? details->getFullName() : "Empty")
                << "computing " << t << " minus "
                << this->map_[source->getFullPath()].back() << endl;

        // update the RTT value for this req-response
        this->map_[source->getFullPath()].back ()
                = t - this->map_[source->getFullPath()].back();


        // increment the number of requests completed so far
        this->requestsCompleted_ ++;

        // check if we have reached the end
        if (this->requestsCompleted_ == this->totalClientRequests_) {
            EV << "=== Coordinator::receiveSignal: all client requests completed. "
                    << "Calling finish" << endl;
            getSimulation()->callFinish(); // call finish recursively on everything

            // stop the simulation
            endSimulation();
        }
    } else {
        throw cRuntimeError("Coordinator::receiveSignal -- bad signal ID received");
    }
}

void Coordinator::finish(cComponent *component, simsignal_t id)
{
    EV << "=== Coordinator::finish (listener method) "
            << "\tComp = " << component->getName ()
            << "\tSignal ID = " << id
            << "\tSignal Name = " << getSignalName (id)
            << endl;
}


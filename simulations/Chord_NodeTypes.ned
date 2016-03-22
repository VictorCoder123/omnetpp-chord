//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

// In this file describe all the node types we have in our system
package CS6381_Chord_P2P.simulations;

import inet.node.inet.StandardHost;

// here we define a client node
module ClientHost extends StandardHost
{
    parameters:
        @display("i=device/pc");
        numTcpApps = 1; // this client supports only one TCP App
}

// Here we define the physical host on which the Chord logic resides.
//
module ChordHost extends StandardHost
{
    parameters:
        @display("i=device/server");
        numTcpApps = 1;  // this server node supports only one type of TCP App
}

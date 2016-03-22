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

package CS6381_Chord_P2P.simulations;

// channel defn
import ned.DatarateChannel;

// in this file we define our channel types we are going to use in our simulations
channel EthernetCable extends DatarateChannel
{     
    datarate = default(100Mbps);
	delay = default(0.1us);
	ber = default (1.0e-5);	// bit error rate
}

channel FiberLine extends DatarateChannel
{
    delay = default(1us);
    datarate = default(512Mbps);
}


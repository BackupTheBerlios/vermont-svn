/**************************************************************************/
/*  NASTY - Network Analysis and STatistics Yielding                      */
/*                                                                        */
/*  Copyright (C) 2006 History Project, http://www.history-project.net    */
/*  History (HIgh-Speed neTwork mOniToring and analYsis) is a research    */
/*  project by the Universities of Tuebingen and Erlangen-Nuremberg,      */
/*  Germany                                                               */
/*                                                                        */
/*  Authors of NASTY are:                                                 */
/*      Christian Japes, University of Erlangen-Nuremberg                 */
/*      Thomas Schurtz, University of Tuebingen                           */
/*      David Halsband, University of Tuebingen                           */
/*      Gerhard Muenz <muenz@informatik.uni-tuebingen.de>,                */
/*          University of Tuebingen                                       */
/*      Falko Dressler <dressler@informatik.uni-erlangen.de>,             */
/*          University of Erlangen-Nuremberg                              */
/*                                                                        */
/*  This program is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU General Public License as published by  */
/*  the Free Software Foundation; either version 2 of the License, or     */
/*  (at your option) any later version.                                   */
/*                                                                        */
/*  This program is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*  GNU General Public License for more details.                          */
/*                                                                        */
/*  You should have received a copy of the GNU General Public License     */
/*  along with this program; if not, write to the Free Software           */
/*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */
/**************************************************************************/

package de.japes.net.nasty.collector;

public class Identifier {
	
	private long id;
	
	public Identifier() {};
	
	public Identifier (long id) {
		
		this.id = id;
	}
	
	public long getID() {
		return id;
	}
	public void setID(long id) {
		this.id = id;
	}
	public int hashCode() {
		return (int)id;
	}
	public boolean equals(Object o) {
		
		return (o instanceof Identifier) &&
			this.id == ((Identifier)o).id;
	}
}

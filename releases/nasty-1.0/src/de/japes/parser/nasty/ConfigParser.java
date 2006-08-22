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

/* Generated By:JavaCC: Do not edit this line. ConfigParser.java */
package de.japes.parser.nasty;

import java.util.HashMap;

public class ConfigParser implements ConfigParserConstants {

        private static int udpBufferSize=0;
        private static String dbServer="";
        private static short dbPort=0;
        private static String dbName="";
        private static String dbUser="";
        private static String dbPass="";
        private static short daysDetailedData=0;
        private static short weeksDailyData=0;
        private static boolean initialized = false;

        public static void main(String args[]) throws ParseException {

                ConfigParser confParser = new ConfigParser(System.in);
                confParser.Start();
        }

        private static void setUdpBufferSize(String buffer) {
                udpBufferSize = Integer.parseInt(buffer);
        }

        private static int getUdpBufferSize() {
                return udpBufferSize;
        }

        private static void setDBServer(String server) {
                dbServer = server;
        }

        public static String getDBServer() {
                return dbServer;
        }

        private static void setDBPort(String port) {
                dbPort = Short.parseShort(port);
        }

        public static short getDBPort() {
                return dbPort;
        }

        public static void setDBUser(String user) {
                dbUser = user;
        }

        public static String getDBUser() {
                return dbUser;
        }

        public static void setDBPass(String pass) {
                dbPass = pass;
        }

        public static String getDBPass() {
                return dbPass;
        }

        private static void setDBName(String name) {
                dbName = name;
        }

        public static String getDBName() {
                return dbName;
        }

        private static void setDaysDetailedData(String days) {
                daysDetailedData = Short.parseShort(days);
        }

        public static short getDaysDetailedData() {

                //detailed data for at least 24h
                if (daysDetailedData<1)
                        return 1;
                else
                        return daysDetailedData;
        }

        private static void setWeeksDailyData(String weeks) {
                weeksDailyData = Short.parseShort(weeks);
        }

        public static short getWeeksDailyData() {

                //daily data for at least one week
                if (weeksDailyData<1)
                        return 1;
                else
                        return weeksDailyData;
        }

        public static boolean isInitialized() {
                return initialized;
        }

  static final public void Start() throws ParseException {
    ConfigFile();
    jj_consume_token(0);
                            initialized=true;
  }

  static final public void ConfigFile() throws ParseException {
    label_1:
    while (true) {
      switch ((jj_ntk==-1)?jj_ntk():jj_ntk) {
      case UDP_BUFFER_SIZE:
      case DATABASE_NAME:
      case DATABASE_SERVER:
      case DATABASE_PORT:
      case DATABASE_USER:
      case DATABASE_PASS:
      case DAYS_DETAILED_DATA:
      case WEEKS_DAILY_DATA:
        ;
        break;
      default:
        jj_la1[0] = jj_gen;
        break label_1;
      }
      Line();
    }
  }

  static final public void Line() throws ParseException {
        Token t;
    switch ((jj_ntk==-1)?jj_ntk():jj_ntk) {
    case UDP_BUFFER_SIZE:
      jj_consume_token(UDP_BUFFER_SIZE);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                  setUdpBufferSize(t.image);
      break;
    case DATABASE_SERVER:
      jj_consume_token(DATABASE_SERVER);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                    setDBServer(t.image);
      break;
    case DATABASE_PORT:
      jj_consume_token(DATABASE_PORT);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                  setDBPort(t.image);
      break;
    case DATABASE_NAME:
      jj_consume_token(DATABASE_NAME);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                  setDBName(t.image);
      break;
    case DATABASE_USER:
      jj_consume_token(DATABASE_USER);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                  setDBUser(t.image);
      break;
    case DATABASE_PASS:
      jj_consume_token(DATABASE_PASS);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                  setDBPass(t.image);
      break;
    case DAYS_DETAILED_DATA:
      jj_consume_token(DAYS_DETAILED_DATA);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                       setDaysDetailedData(t.image);
      break;
    case WEEKS_DAILY_DATA:
      jj_consume_token(WEEKS_DAILY_DATA);
      jj_consume_token(WHITESPACE);
      t = jj_consume_token(VALUE);
                                                     setWeeksDailyData(t.image);
      break;
    default:
      jj_la1[1] = jj_gen;
      jj_consume_token(-1);
      throw new ParseException();
    }
  }

  static private boolean jj_initialized_once = false;
  static public ConfigParserTokenManager token_source;
  static SimpleCharStream jj_input_stream;
  static public Token token, jj_nt;
  static private int jj_ntk;
  static private int jj_gen;
  static final private int[] jj_la1 = new int[2];
  static private int[] jj_la1_0;
  static {
      jj_la1_0();
   }
   private static void jj_la1_0() {
      jj_la1_0 = new int[] {0x3fc0,0x3fc0,};
   }

  public ConfigParser(java.io.InputStream stream) {
    if (jj_initialized_once) {
      System.out.println("ERROR: Second call to constructor of static parser.  You must");
      System.out.println("       either use ReInit() or set the JavaCC option STATIC to false");
      System.out.println("       during parser generation.");
      throw new Error();
    }
    jj_initialized_once = true;
    jj_input_stream = new SimpleCharStream(stream, 1, 1);
    token_source = new ConfigParserTokenManager(jj_input_stream);
    token = new Token();
    jj_ntk = -1;
    jj_gen = 0;
    for (int i = 0; i < 2; i++) jj_la1[i] = -1;
  }

  static public void ReInit(java.io.InputStream stream) {
    jj_input_stream.ReInit(stream, 1, 1);
    token_source.ReInit(jj_input_stream);
    token = new Token();
    jj_ntk = -1;
    jj_gen = 0;
    for (int i = 0; i < 2; i++) jj_la1[i] = -1;
  }

  public ConfigParser(java.io.Reader stream) {
    if (jj_initialized_once) {
      System.out.println("ERROR: Second call to constructor of static parser.  You must");
      System.out.println("       either use ReInit() or set the JavaCC option STATIC to false");
      System.out.println("       during parser generation.");
      throw new Error();
    }
    jj_initialized_once = true;
    jj_input_stream = new SimpleCharStream(stream, 1, 1);
    token_source = new ConfigParserTokenManager(jj_input_stream);
    token = new Token();
    jj_ntk = -1;
    jj_gen = 0;
    for (int i = 0; i < 2; i++) jj_la1[i] = -1;
  }

  static public void ReInit(java.io.Reader stream) {
    jj_input_stream.ReInit(stream, 1, 1);
    token_source.ReInit(jj_input_stream);
    token = new Token();
    jj_ntk = -1;
    jj_gen = 0;
    for (int i = 0; i < 2; i++) jj_la1[i] = -1;
  }

  public ConfigParser(ConfigParserTokenManager tm) {
    if (jj_initialized_once) {
      System.out.println("ERROR: Second call to constructor of static parser.  You must");
      System.out.println("       either use ReInit() or set the JavaCC option STATIC to false");
      System.out.println("       during parser generation.");
      throw new Error();
    }
    jj_initialized_once = true;
    token_source = tm;
    token = new Token();
    jj_ntk = -1;
    jj_gen = 0;
    for (int i = 0; i < 2; i++) jj_la1[i] = -1;
  }

  public void ReInit(ConfigParserTokenManager tm) {
    token_source = tm;
    token = new Token();
    jj_ntk = -1;
    jj_gen = 0;
    for (int i = 0; i < 2; i++) jj_la1[i] = -1;
  }

  static final private Token jj_consume_token(int kind) throws ParseException {
    Token oldToken;
    if ((oldToken = token).next != null) token = token.next;
    else token = token.next = token_source.getNextToken();
    jj_ntk = -1;
    if (token.kind == kind) {
      jj_gen++;
      return token;
    }
    token = oldToken;
    jj_kind = kind;
    throw generateParseException();
  }

  static final public Token getNextToken() {
    if (token.next != null) token = token.next;
    else token = token.next = token_source.getNextToken();
    jj_ntk = -1;
    jj_gen++;
    return token;
  }

  static final public Token getToken(int index) {
    Token t = token;
    for (int i = 0; i < index; i++) {
      if (t.next != null) t = t.next;
      else t = t.next = token_source.getNextToken();
    }
    return t;
  }

  static final private int jj_ntk() {
    if ((jj_nt=token.next) == null)
      return (jj_ntk = (token.next=token_source.getNextToken()).kind);
    else
      return (jj_ntk = jj_nt.kind);
  }

  static private java.util.Vector jj_expentries = new java.util.Vector();
  static private int[] jj_expentry;
  static private int jj_kind = -1;

  static public ParseException generateParseException() {
    jj_expentries.removeAllElements();
    boolean[] la1tokens = new boolean[17];
    for (int i = 0; i < 17; i++) {
      la1tokens[i] = false;
    }
    if (jj_kind >= 0) {
      la1tokens[jj_kind] = true;
      jj_kind = -1;
    }
    for (int i = 0; i < 2; i++) {
      if (jj_la1[i] == jj_gen) {
        for (int j = 0; j < 32; j++) {
          if ((jj_la1_0[i] & (1<<j)) != 0) {
            la1tokens[j] = true;
          }
        }
      }
    }
    for (int i = 0; i < 17; i++) {
      if (la1tokens[i]) {
        jj_expentry = new int[1];
        jj_expentry[0] = i;
        jj_expentries.addElement(jj_expentry);
      }
    }
    int[][] exptokseq = new int[jj_expentries.size()][];
    for (int i = 0; i < jj_expentries.size(); i++) {
      exptokseq[i] = (int[])jj_expentries.elementAt(i);
    }
    return new ParseException(token, exptokseq, tokenImage);
  }

  static final public void enable_tracing() {
  }

  static final public void disable_tracing() {
  }

}
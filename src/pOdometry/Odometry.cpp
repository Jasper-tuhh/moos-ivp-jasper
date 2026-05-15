/************************************************************/
/*    NAME: jasper                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "Odometry.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

Odometry::Odometry()		// Constructor edited 
  : m_first_reading(true),      // first reading is initialised as true and set to false after first reading
    m_current_x(0),             // all postions are initialised as zero and filled with values from moos
    m_previous_x(0),
    m_current_y(0),
    m_previous_y(0),
    m_total_distance(0),
    m_current_depth(0),
    m_depth_distance(0),
    depth_thresh(25)
{
}

//---------------------------------------------------------
// Destructor

Odometry::~Odometry()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool Odometry::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

#if 0 // Keep these around just for template
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString(); 
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

     if(key == "NAV_DEPTH") 
       	m_current_depth = msg.GetDouble(); // line edited
     else if(key == "NAV_X") 
       	m_current_x = msg.GetDouble();      // if the keyword is NAV X the current x position is overloaded by the new value
     else if (key == "NAV_Y")
	m_current_y = msg.GetDouble();      // if the keyword is NAV Y the current y position is overloaded by the new value
     else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool Odometry::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool Odometry::Iterate()
{
  AppCastingMOOSApp::Iterate();
  
  // Updating all parameters unless it's the first reading
  
  if(m_first_reading) {
  	m_previous_x = m_current_x;
        m_previous_y = m_current_y;
        m_first_reading = false;}
  else { 
  	double dx = m_current_x - m_previous_x;                     // calculate difference between current and previous position
        double dy = m_current_y - m_previous_y;
        double delta_distance = sqrt(dx*dx + dy*dy);

        m_total_distance = m_total_distance + 2 * delta_distance;   // due to the tick rate problem mentioned in the exc description the total distance is updated with two delta 
        if (depth_thresh < m_current_depth)
            m_depth_distance = m_depth_distance + 2 * delta_distance;
        
        m_previous_x = m_current_x;                                 // update previous values
        m_previous_y = m_current_y;
    }
        
        Notify("ODOMETRY_DIST", m_total_distance);
        Notify("ODOMETRY_DIST_AT_DEPTH", m_depth_distance);
        
        if(m_depth_distance >= m_dist_thresh) {
        Notify("RETURN", "true");
        // Notify("DEPLOY", "false");
    }
        
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Odometry::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;
    if(param == "foo") {
      handled = true;
    }
    else if(param == "bar") {
      handled = true;
    }
    
    if(param == "depth_thresh") {
    depth_thresh = atof(value.c_str());
    handled = true;
    }
    
    else if(param == "dist_thresh") {
        m_dist_thresh = atof(value.c_str());
        handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);

  }
  
  Notify("ODOMETRY_DIST", 0.0);
  Notify("ODOMETRY_DIST_AT_DEPTH", 0.0);
  
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void Odometry::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_X", 0);		// Line edited
  Register("NAV_Y", 0);		// Line edited
  Register("NAV_DEPTH", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool Odometry::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << "File:                                       " << endl;
  m_msgs << "============================================" << endl;

  ACTable actab(4);
  actab << "Alpha | Bravo | Charlie | Delta";
  actab.addHeaderLines();
  actab << "one" << "two" << "three" << "four";
  m_msgs << actab.getFormattedString();
  
  m_msgs << "Total Distance Traveled: " << m_total_distance << endl;

  return(true);
}





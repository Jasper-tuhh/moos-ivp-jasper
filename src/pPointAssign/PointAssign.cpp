/************************************************************/
/*    NAME: Jasper                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.cpp                                        */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "ACTable.h"
#include "PointAssign.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

PointAssign::PointAssign()
{
  m_firstpoint_received = false;
  m_lastpoint_received  = false;
  m_assign_index        = 0;
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
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

      if(key == "VISIT_POINT") {
       string sval = msg.GetString();
      
      if(sval == "firstpoint") {
        m_firstpoint_received = true;
        m_points.clear();
        m_assign_index = 0;
      }
      else if(sval == "lastpoint") {
        m_lastpoint_received = true;
      }
      else {
       
    string xstr = tokStringParse(sval, "x", ',', '=');
    string ystr = tokStringParse(sval, "y", ',', '=');
    string idstr = tokStringParse(sval, "id", ',', '=');
    
    double x = stod(xstr);
    double y = stod(ystr);
    
    int v = m_assign_index % 2;
    string vname = m_vehicle_names[v];
    m_assign_index++;
    
    string color = "yellow";  // default
    
    if(vname == "henry")
      color = "yellow";
    else if(vname == "gilda")
      color = "cyan";

    string msg_out = "points=" + xstr + "," + ystr;
    Notify("VISIT_POINT_" + toupper(vname), msg_out);
    postViewPoint(x, y, idstr, color);
    }
   }
     else if(key != "APPCAST_REQ") // handled by AppCastingMOOSApp
       reportRunWarning("Unhandled Mail: " + key);
   }
	
   return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool PointAssign::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();
  // Do your thing here!
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool PointAssign::OnStartUp()
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
    if(param == "vehicle_name") {
    m_vehicle_names.push_back(value);
    handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }
  
  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);
}

void PointAssign::postViewPoint(double x, double y, string label, string color)
 {
   XYPoint point(x, y);
   point.set_label(label);
   point.set_color("vertex", color);  // yellow is handy on dark screen 
   point.set_param("vertex_size", "4");

   string spec = point.get_spec();    // gets the string representation of a point
   Notify("VIEW_POINT", spec);
 }

//------------------------------------------------------------
// Procedure: buildReport()

bool PointAssign::buildReport()
{
  m_msgs << "====================================" << endl;
  m_msgs << "Points received: " << m_assign_index  << endl;
  m_msgs << "First point received: " << m_firstpoint_received << endl;
  m_msgs << "Last point received:  " << m_lastpoint_received  << endl;
  m_msgs << "====================================" << endl;

  ACTable actab(2);
  actab << "Vehicle | Points Assigned";
  actab.addHeaderLines();

  for(int i=0; i<m_vehicle_names.size(); i++) {
    int count = 0;
    for(int j=i; j<m_assign_index; j+=m_vehicle_names.size())
      count++;
    actab << m_vehicle_names[i] << count;
  }
  m_msgs << actab.getFormattedString();
  return(true);
}





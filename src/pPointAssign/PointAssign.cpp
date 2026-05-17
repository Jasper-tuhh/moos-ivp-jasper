/************************************************************/
/*    NAME: Jasper                                          */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: PointAssign.cpp                                 */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include <algorithm>
#include <cctype>
#include <string>
#include "XYPoint.h"
#include "MBUtils.h"
#include "ACTable.h"
#include "PointAssign.h"

using namespace std;

std::string to_upper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return s;
}

//---------------------------------------------------------
// Constructor()

PointAssign::PointAssign()
{
  m_firstpoint_received = false;
  m_lastpoint_received  = false;
  m_assign_index        = 0;
  m_assign_by_region    = false; // Ensure this is initialized
}

//---------------------------------------------------------
// Destructor

PointAssign::~PointAssign()
{
}

//---------------------------------------------------------
bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);
  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();

    if(key == "VISIT_POINT") {
      string sval = msg.GetString();
      
      // Guard against an empty configuration using your matching header variable
      if(m_vehicle_names.empty()) {
        reportRunWarning("No vehicles configured via 'vname' or 'vehicle_name' in .moos file!");
        continue;
      }

      //---------------------------------------------------------
      // 1. Handle "firstpoint"
      //---------------------------------------------------------
      if(sval == "firstpoint") {
        m_firstpoint_received = true;
        m_view_points.clear();
        m_assign_index = 0;

        // Forward "firstpoint" to ALL registered vehicles exactly as required
        for(unsigned int i=0; i<m_vehicle_names.size(); i++) {
          string vname_upper = to_upper(m_vehicle_names[i]);
          Notify("VISIT_POINT_" + vname_upper, "firstpoint");
        }
      }
      //---------------------------------------------------------
      // 2. Handle "lastpoint"
      //---------------------------------------------------------
      else if(sval == "lastpoint") {
        m_lastpoint_received = true;

        for(unsigned int i=0; i<m_vehicle_names.size(); i++) {
          string vname_upper = to_upper(m_vehicle_names[i]);
          Notify("VISIT_POINT_" + vname_upper, "lastpoint");
        }
      }
        
      //---------------------------------------------------------
      // 3. Handle coordinate points
      //---------------------------------------------------------
      else {
        string xstr  = tokStringParse(sval, "x", ',', '=');
        string ystr  = tokStringParse(sval, "y", ',', '=');
        string idstr = tokStringParse(sval, "id", ',', '=');

        if(xstr.empty() || ystr.empty()) continue;

        double x = stod(xstr);
        double y = stod(ystr);
        
        string target_vname = "";
        string color = "yellow";

        // Determine assignment logic (By Region vs Alternating)
        if(m_assign_by_region) {
          // East-West split. Total X-span is -25 to 200. Middle line is x = 87.5
          if(x < 87.5) {
            target_vname = m_vehicle_names[0]; // West vehicle (typically henry)
            color = "yellow";
          } else {
            // Check if a second vehicle exists, otherwise fallback to the first
            target_vname = (m_vehicle_names.size() > 1) ? m_vehicle_names[1] : m_vehicle_names[0]; // East vehicle (typically gilda)
            color = "cyan";
          }
        } 
        else {
          unsigned int v = m_assign_index % m_vehicle_names.size();
          target_vname = m_vehicle_names[v];
          m_assign_index++;
          
          color = (v == 0) ? "yellow" : "cyan";
        }

        string target_vname_upper = to_upper(target_vname);
        string moos_var = "VISIT_POINT_" + target_vname_upper;

        // Publish the coordinate to the targeted vehicle
        string msg_out = "x=" + xstr + ",y=" + ystr + ",id=" + idstr;
        Notify(moos_var, msg_out);

        // Render point inside MarineViewer locally on shoreside
        XYPoint point(x, y);
        point.set_label(idstr);
        point.set_color("vertex", color);
        point.set_param("vertex_size", "4");
        m_view_points.push_back(point);
        Notify("VIEW_POINT", point.get_spec());
      }
    }
    else if(key != "APPCAST_REQ") {
      reportRunWarning("Unhandled Mail: " + key);
    }
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

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();

  for(unsigned int i=0; i<m_view_points.size(); i++) {
    Notify("VIEW_POINT", m_view_points[i].get_spec());
  }

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool PointAssign::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);

  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  for(string s : sParams) {
    string orig  = s;
    string param = tolower(biteStringX(s, '='));
    string value = s;
    bool handled = false;

    // Supports both 'vname' and legacy 'vehicle_name' parameter names
    if(param == "vname" || param == "vehicle_name") {
      m_vehicle_names.push_back(value);
      handled = true;
    }
    else if(param == "assign_by_region") {
      m_assign_by_region = (tolower(value) == "true");
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
   point.set_color("vertex", color);
   point.set_param("vertex_size", "4");

   string spec = point.get_spec();
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

  for(unsigned int i=0; i<m_vehicle_names.size(); i++) {
    int count = 0;
    if (m_assign_by_region) {
      actab << m_vehicle_names[i] << "See Visuals / Log";
    } else {
      for(unsigned int j=i; j<m_assign_index; j+=m_vehicle_names.size())
        count++;
      actab << m_vehicle_names[i] << count;
    }
  }
  m_msgs << actab.getFormattedString();
  return(true);
}

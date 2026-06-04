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

// custom function to create upper case string variables
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
  m_assign_by_region    = false; 
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

// When new messages arrive in the MOOSDB the content gets checked for the 
// indicator "VISIT_POINT", first and last point get special treatment
    if(key == "VISIT_POINT") {
      string sval = msg.GetString();
      
      if(sval == "firstpoint") {
        m_firstpoint_received = true;
        m_view_points.clear();
        m_assign_index = 0;
        m_vehicle_counts.assign(m_vnames.size(), 0);
        
// Custom Function to_upper is invoked to notify the right vessel with the 
// respective points
        for(unsigned int i=0; i<m_vnames.size(); i++) {
          Notify("VISIT_POINT_" + to_upper(m_vnames[i]), "firstpoint");
        }
      }

      else if(sval == "lastpoint") {
        m_lastpoint_received = true;

        for(unsigned int i=0; i<m_vnames.size(); i++) {
          Notify("VISIT_POINT_" + to_upper(m_vnames[i]), "lastpoint");
        }
      }
      
// coordinates are parsed with respect to their x, y and id
      else {
        string xstr  = tokStringParse(sval, "x", ',', '=');
        string ystr  = tokStringParse(sval, "y", ',', '=');
        string idstr = tokStringParse(sval, "id", ',', '=');

        if(xstr.empty() || ystr.empty()) continue;

// The parsed value possesses the wrong type, which is conversed to double
        double x = stod(xstr);
        double y = stod(ystr);
        
        string target_vname = "";
        string color = "yellow";
        
        int chosen_index = 0;
        
// Depending on the assign by region value provided in the configuration the
// coloration is determined
        if(m_assign_by_region) {
          if(x < 87.5) {
            chosen_index = 0; 
              target_vname = m_vnames[chosen_index]; 
              color = "yellow";
          } else {
            chosen_index = (m_vnames.size() > 1) ? 1 : 0; 
              target_vname = m_vnames[chosen_index]; 
              color = "cyan";
          }
        } 
        else {
          chosen_index = m_assign_index % m_vnames.size();
          target_vname = m_vnames[chosen_index];
          m_assign_index++;
          color = (chosen_index == 0) ? "yellow" : "cyan";
        }

        m_vehicle_counts[chosen_index]++;

// VISIT_POINT gets assigned a vessel with upper case function
        string moos_var = "VISIT_POINT_" + to_upper(target_vname);
        string msg_out = "x=" + xstr + ",y=" + ystr + ",id=" + idstr;
        Notify(moos_var, msg_out);

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
bool PointAssign::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
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

    if(param == "vname") {
      m_vnames.push_back(value);
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
void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);
}

//---------------------------------------------------------
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
bool PointAssign::buildReport()
{
  m_msgs << "====================================" << endl;
  m_msgs << "Points received: " << m_assign_index << endl;
  m_msgs << "First point received: " << m_firstpoint_received << endl;
  m_msgs << "Last point received:  " << m_lastpoint_received << endl;
  m_msgs << "====================================" << endl;

// AppCasting Table to show in the terminal, with the vehicle and the points

  ACTable actab(2);
  actab << "Vehicle | Points Assigned";
  actab.addHeaderLines();

  if(m_vehicle_counts.size() == m_vnames.size()) {
    for(unsigned int i=0; i<m_vnames.size(); i++) {
      actab << m_vnames[i] << m_vehicle_counts[i];
    }
  } else {
    for(unsigned int i=0; i<m_vnames.size(); i++) {
      actab << m_vnames[i] << 0;
    }
  }

  m_msgs << actab.getFormattedString();
  return(true);
}

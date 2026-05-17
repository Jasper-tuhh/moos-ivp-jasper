#include <iterator>
#include <algorithm>
#include <cmath>
#include "MBUtils.h"
#include "ACTable.h"
#include "GenPath.h"
#include "XYSegList.h"

using namespace std;

//---------------------------------------------------------
GenPath::GenPath()
{
  m_firstpoint_received = false;
  m_lastpoint_received  = false;
  m_nav_x = 0.0;
  m_nav_y = 0.0;
}

GenPath::~GenPath() {}

//---------------------------------------------------------
bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();
    string sval   = msg.GetString();

    // 1. Capture vehicle's current position
    if(key == "NAV_X") {
      m_nav_x = msg.GetDouble();
    }
    else if(key == "NAV_Y") {
      m_nav_y = msg.GetDouble();
    }
    
    // 2. Process incoming points from shoreside pPointAssign
    else if(key == "VISIT_POINT") {
      if(sval == "firstpoint") {
        m_firstpoint_received = true;
        m_lastpoint_received  = false;
        m_received_points.clear();
      }
      else if(sval == "lastpoint") {
        m_lastpoint_received = true;
        // All points are in! Calculate optimization path
        generateAndPublishPath();
      }
      else {
        // Parse coordinate data: "x=X,y=Y,id=ID"
        string xstr  = tokStringParse(sval, "x", ',', '=');
        string ystr  = tokStringParse(sval, "y", ',', '=');
        string idstr = tokStringParse(sval, "id", ',', '=');

        if(!xstr.empty() && !ystr.empty()) {
          XYPoint new_pt(stod(xstr), stod(ystr));
          new_pt.set_label(idstr);
          m_received_points.push_back(new_pt);
        }
      }
    }
    else if(key != "APPCAST_REQ") {
      reportRunWarning("Unhandled Mail: " + key);
    }
  }
   
  return(true);
}

//---------------------------------------------------------
void GenPath::generateAndPublishPath()
{
  if(m_received_points.empty()) return;

  XYSegList optimized_seglist;
  vector<XYPoint> remaining = m_received_points;

  // Start optimization sorting from where the vehicle currently sits
  double cur_x = m_nav_x;
  double cur_y = m_nav_y;

  // Greedy TSP Algorithm Loop
  while(!remaining.empty()) {
    int best_index = 0;
    double min_dist = -1.0;

    for(unsigned int i = 0; i < remaining.size(); i++) {
      double dx = remaining[i].x() - cur_x;
      double dy = remaining[i].y() - cur_y;
      double dist = sqrt(dx*dx + dy*dy);

      if(min_dist < 0 || dist < min_dist) {
        min_dist = dist;
        best_index = i;
      }
    }

    // Append nearest unvisited node to the tour geometry
    optimized_seglist.add_vertex(remaining[best_index].x(), remaining[best_index].y());
    
    // Update current position stepping stone
    cur_x = remaining[best_index].x();
    cur_y = remaining[best_index].y();

    // Erase it from unvisited cache pool
    remaining.erase(remaining.begin() + best_index);
  }

  // Package tour specification as an update parameter string for BHV_Waypoint
  string update_str = "points = " + optimized_seglist.get_spec();
  Notify("WPT_UPDATE", update_str);
}

//---------------------------------------------------------
bool GenPath::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------
bool GenPath::Iterate()
{
  AppCastingMOOSApp::Iterate();
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
bool GenPath::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  registerVariables();     
  return(true);
}

//---------------------------------------------------------
void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("VISIT_POINT", 0);
}

//------------------------------------------------------------
bool GenPath::buildReport() 
{
  m_msgs << "============================================" << endl;
  m_msgs << " Points Stored: " << m_received_points.size() << endl;
  m_msgs << " Sequence Ready: " << (m_lastpoint_received ? "YES" : "NO") << endl;
  m_msgs << "============================================" << endl;
  return(true);
}

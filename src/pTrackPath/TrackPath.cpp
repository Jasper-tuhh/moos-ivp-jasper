#include <iterator>
#include <algorithm>
#include <cmath>
#include "MBUtils.h"
#include "ACTable.h"
#include "TrackPath.h"
#include "XYSegList.h"

using namespace std;

//---------------------------------------------------------
TrackPath::TrackPath()
{
  m_firstpoint_received = false;
  m_lastpoint_received  = false;
  // new variable with default value 5m 
  m_visit_radius       = 5.0;
  // new boolean for regenerating path
  m_regenerate_pending = false;
  m_nav_x = 0.0;
  m_nav_y = 0.0;
}

TrackPath::~TrackPath() {}

//---------------------------------------------------------
bool TrackPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p=NewMail.begin(); p!=NewMail.end(); p++) {
    CMOOSMsg &msg = *p;
    string key    = msg.GetKey();
    string sval   = msg.GetString();
// Similar to generate path
    if(key == "NAV_X") {
      m_nav_x = msg.GetDouble();
      checkVisits();
    }
    else if(key == "NAV_Y") {
      m_nav_y = msg.GetDouble();
    }
// New variable, for the message GENPATH_REGENERATE 
// the bool is set to true
    else if(key == "GENPATH_REGENERATE") {
    m_regenerate_pending = true;
    }
    
// first point sets bool for later report
    else if(key == "VISIT_POINT") {
      if(sval == "firstpoint") {
        m_firstpoint_received = true;
        m_lastpoint_received  = false;
        m_unvisited_points.clear();
      }
// last point sets bool for later report
      else if(sval == "lastpoint") {
        m_all_points      = m_unvisited_points ;
        
        generateAndPublishPath();
      }
// parse coordinates
      else {
        // parse coordinate data: "x=X,y=Y,id=ID"
        string xstr  = tokStringParse(sval, "x", ',', '=');
        string ystr  = tokStringParse(sval, "y", ',', '=');
        string idstr = tokStringParse(sval, "id", ',', '=');
        
// debug (AI - proposal!)
        if(!xstr.empty() && !ystr.empty()) {
          XYPoint new_pt(stod(xstr), stod(ystr));
          new_pt.set_label(idstr);
          m_unvisited_points.push_back(new_pt);
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
// void function calculates path according to greedy algorithm and notifies update
void TrackPath::generateAndPublishPath()
{
  if(m_unvisited_points.empty()) return;
  
// similar to the path generation
  XYSegList optimized_seglist;
  vector<XYPoint> remaining = m_unvisited_points;

  double cur_x = m_nav_x;
  double cur_y = m_nav_y;

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
// (see path generation)
    optimized_seglist.add_vertex(remaining[best_index].x(), remaining[best_index].y());
    
    cur_x = remaining[best_index].x();
    cur_y = remaining[best_index].y();

    remaining.erase(remaining.begin() + best_index);
  }
// Updates waypoint with parsed coordinates
  string update_str = "points = " + optimized_seglist.get_spec();
  Notify("WPT_UPDATE", update_str);
}

// void function calculates distances between current position and remaining
// points
void TrackPath::checkVisits()
{
  vector<XYPoint> still_unvisited;
  for(auto& pt : m_unvisited_points) {
    double dx   = pt.x() - m_nav_x;
    double dy   = pt.y() - m_nav_y;
    double dist = sqrt(dx*dx + dy*dy);
    if(dist > m_visit_radius)
      still_unvisited.push_back(pt);
  }
  m_unvisited_points = still_unvisited;
}

//---------------------------------------------------------
bool TrackPath::OnConnectToServer()
{
   registerVariables();
   return(true);
}

//---------------------------------------------------------

// bool checks if regeneration is necessary, if yes set to false and proceed 
// with updating  if there are no points, that have still not been visited 
// -> then set GENPATH to complete, else publish the path, that is determined 
// with generate function

bool TrackPath::Iterate()
{
    AppCastingMOOSApp::Iterate();

  if(m_regenerate_pending) {
    m_regenerate_pending = false;
    if(m_unvisited_points.empty()) {
      Notify("GENPATH_COMPLETE", "true");  
    } else {
      
      generateAndPublishPath();
    }
  }

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
bool TrackPath::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());
    
  STRING_LIST::iterator q;
  for(q = sParams.begin(); q != sParams.end(); q++) {
    string orig  = *q;
    string param = toupper(biteStringX(orig, '='));
    string value = orig;
    
// New Radius variable, that is parsed from string to double 
    if(param == "VISIT_RADIUS")
      m_visit_radius = stod(value);
    else
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();     
  return(true);
}

//---------------------------------------------------------
void TrackPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("VISIT_POINT", 0);
  Register("GENPATH_REGENERATE",0);
}

//------------------------------------------------------------
bool TrackPath::buildReport() 
{
  m_msgs << "============================================\n";
    m_msgs << " Visit radius : " << m_visit_radius << " m\n";
    m_msgs << " Total points : " << m_all_points.size()      << "\n";
    m_msgs << " Unvisited    : " << m_unvisited_points.size() << "\n";
    m_msgs << " Visited      : " << m_all_points.size() - m_unvisited_points.size() << "\n";
    m_msgs << "============================================\n";
    return(true);
}

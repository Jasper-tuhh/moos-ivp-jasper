#ifndef TrackPath_HEADER
#define TrackPath_HEADER
#include "XYPoint.h"
#include "XYSegList.h"
#include <vector>

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class TrackPath : public AppCastingMOOSApp
{
public:
  TrackPath();
  ~TrackPath();

protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();
  bool buildReport();
  void registerVariables();
  
  void generateAndPublishPath();
  void checkVisits();

private: // State variables
  bool   m_firstpoint_received;
  bool   m_lastpoint_received;
  bool   m_regenerate_pending;
  
  double m_nav_x;
  double m_nav_y;
  double m_visit_radius;

  std::vector<XYPoint> m_all_points;
  std::vector<XYPoint> m_unvisited_points;
  std::vector<XYPoint> m_current_path;
};

#endif

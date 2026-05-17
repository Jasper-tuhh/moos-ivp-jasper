#ifndef GenPath_HEADER
#define GenPath_HEADER
#include "XYPoint.h"
#include "XYSegList.h"
#include <vector>

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class GenPath : public AppCastingMOOSApp
{
public:
  GenPath();
  ~GenPath();

protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();
  bool buildReport();
  void registerVariables();
  
  // Custom method
  void generateAndPublishPath();

private: // State variables
  bool   m_firstpoint_received;
  bool   m_lastpoint_received;
  
  double m_nav_x;
  double m_nav_y;

  std::vector<XYPoint> m_received_points; 
};

#endif

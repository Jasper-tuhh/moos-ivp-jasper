#--------------------------------------------------------
# The CMakeLists.txt for:                       pTrackPath
# Author(s):                              Jasper
#--------------------------------------------------------

SET(SRC
  TrackPath.cpp
  TrackPath_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pTrackPath ${SRC})

TARGET_LINK_LIBRARIES(pTrackPath
   ${MOOS_LIBRARIES}
   geometry
   apputil
   mbutil
   m
   pthread)


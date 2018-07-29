/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
/*                Copyright (C)2018, WWIV Software Services               */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/**************************************************************************/
#include "sdk/net/callouts.h"

#include "core/log.h"

#include <algorithm>
#include <chrono>
#include <string>

using std::string;
using namespace std::chrono;
using namespace wwiv::core;
using namespace wwiv::sdk;

namespace wwiv {
namespace sdk {
namespace net {

// TODO(rushfan): Move this to core?
template<typename T>
T bytes_to_k(T b) {
  return (b) ? static_cast<T>((b + 1023) / 1024) : static_cast<T>(0);
}

bool allowed_to_call(const net_call_out_rec& con, const DateTime& dt) {

  if (con.options & options_no_call) {
    VLOG(3) << "Not Calling: options_no_call";
    return false;
  }

  auto l = con.min_hr;
  auto h = con.max_hr;
  auto n = dt.hour();
  if (l < 0 || h <= 0 || l == h) {
    return true;
  }
  if (l < h) {
    // Case where we want to allow it from say 0200-2000
    return (n >= l && n < h);
  } else {
    // Case where we want to allow it from say 2000-0200
    return (n >= h && n < l);
  }
}

/**
 * Checks the net_contact_rec and net_call_out_rec to ensure the node specified
 * is ok to call and does not violate any constraints.
 */
bool should_call(const NetworkContact& ncn, const net_call_out_rec& con, const DateTime& dt) {
  VLOG(2) << "should_call: @" << con.sysnum;

  if (!allowed_to_call(con, dt)) {
    VLOG(2) << "!allowed_to_call; skipping";
    return false;
  }

  auto now = dt.to_system_clock();
  if (ncn.bytes_waiting() == 0L && !con.call_every_x_minutes) {
    VLOG(2) << "Skipping: No bytes waiting and !call anyway";
    return false;
  }
  auto min_minutes = std::max<int>(con.call_every_x_minutes, 1);
  auto last_contact = DateTime::from_time_t(ncn.lastcontact()).to_system_clock();
  auto next_contact_time = last_contact + minutes(min_minutes);

  if (con.call_every_x_minutes && now >= next_contact_time) {
    VLOG(1) << "Calling anyway since it's been time: ";
    VLOG(1) << "Last Try: " << DateTime::from_time_t(ncn.lastcontact()).to_string();
    return true;
  }
  if (bytes_to_k(ncn.bytes_waiting()) > con.min_k) {
    VLOG(1) << "Calling: min_k";
    return true;
  }
  VLOG(2) << "Skipping; No reason to call.";
  return false;
}



} // namespace net
} // namespace sdk
} // namespace wwiv

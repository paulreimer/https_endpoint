/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include "test_runner.h"

#include "../src/https_endpoint.h"
#include "../src/tls_connection_interface.h"

// This regex roughly worked to turn C++ class into MAKE_MOCKx() definitions
// Removing the argument names and selecting the correct MACK_MOCKx is needed
// :'<,'>s/virtual \([^ ]\+\) \([^(]\+\)(\([^(]*\)) = 0;/MAKE_MOCK0(\2, \1(\3), override);/
class TLSConnectionMock
: public TLSConnectionInterface
{
public:
  MAKE_MOCK3(initialize, bool(stx::string_view, unsigned short, stx::string_view), override);

  MAKE_MOCK0(init, bool(), override);
  MAKE_MOCK0(clear, bool(), override);

  MAKE_MOCK0(ready, bool(), override);
  MAKE_MOCK0(connected, bool(), override);

  MAKE_MOCK0(store_session, bool(), override);
  MAKE_MOCK0(clear_session, bool(), override);
  MAKE_MOCK0(has_valid_session, bool(), override);

  MAKE_MOCK1(set_verification_level, bool(int), override);
  MAKE_MOCK0(get_verification_level, int(), override);

  MAKE_MOCK2(set_cacert, bool(stx::string_view, bool), override);
  MAKE_MOCK0(clear_cacert, bool(), override);
  MAKE_MOCK0(has_valid_cacert, bool(), override);

  MAKE_MOCK2(connect, bool(stx::string_view, unsigned short), override);
  MAKE_MOCK0(reconnect, bool(), override);
  MAKE_MOCK0(disconnect, bool(), override);

  MAKE_MOCK1(write, int(stx::string_view), override);
  MAKE_MOCK1(read, int(stx::string_view), override);
};

// Explicit template instantiation of mocked class
template class HttpsEndpointAutoConnect<TLSConnectionMock>;

TEST_CASE("Does TLSConnection lifecycle")
{
  TLSConnectionMock conn{};

  REQUIRE_CALL(conn, initialize("www.example.org", 443, "<pem>"))
    .RETURN(true)
    .TIMES(1);

  HttpsEndpointAutoConnect<TLSConnectionMock>(conn, "www.example.org", 443, "<pem>");
}

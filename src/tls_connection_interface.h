/*
 * Copyright Paul Reimer, 2017
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 Unported License.
 * To view a copy of this license, visit
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 * or send a letter to
 * Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#pragma once

#include "stx/string_view.hpp"

class TLSConnectionInterface
{
public:
  virtual ~TLSConnectionInterface() = default;

  virtual bool init() = 0;
  virtual bool clear() = 0;

  virtual bool ready() = 0;
  virtual bool connected() = 0;

  virtual bool store_session() = 0;
  virtual bool clear_session() = 0;
  virtual bool has_valid_session() = 0;

  virtual bool set_verification_level(int level) = 0;
  virtual int get_verification_level() = 0;

  virtual bool set_cacert(
    stx::string_view cacert_pem,
    bool force_disconnect=true
  ) = 0;
  virtual bool clear_cacert() = 0;
  virtual bool has_valid_cacert() = 0;

  virtual bool connect(
    stx::string_view _host,
    const unsigned short _port=443
  ) = 0;

  virtual bool reconnect() = 0;
  virtual bool disconnect() = 0;

  virtual int write(stx::string_view buf) = 0;
  virtual int read(stx::string_view buf) = 0;
};

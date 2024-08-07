/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

class GenericPanel;

// Note: cannot be a Qt object otherwise it will create a compiler ambiguity in the inheriting class
class AutoWidget
{
  friend class GenericPanel;

  public:
    explicit AutoWidget();
    ~AutoWidget();

  protected:
    virtual void updateValue() = 0;

    bool lock();
    void setLock(bool lock);
    void setPanel(GenericPanel * panel);

    void dataChanged();
    bool panelLock();

  private:
    GenericPanel *m_panel;
    bool m_lock;
};

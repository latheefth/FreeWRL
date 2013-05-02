#include "BasePlugin.h"

/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

bool
BasePlugin::HasMethod(NPIdentifier name)
{
  /*return (	name == sFoo_id || 
			name == sShowText_id ||
			name == sClearWindow_id );*/
	return false;
}

/// Returns true if the NPIdentifier passed is managed as a scriptable property
bool
BasePlugin::HasProperty(NPIdentifier name)
{
  /*return (name == sBar_id ||
          name == sPluginType_id);*/
	return false;
}

/// Returns true if the scriptable property is managed and fills the NPVariant pointer with the value
bool
BasePlugin::GetProperty(NPIdentifier name, NPVariant *result)
{
	
  VOID_TO_NPVARIANT(*result);
	
  //VIENE FATTA LA VERIFICA SULLA DERIVAZIONE DELLA CLASSE DA NPOBJECT PER IL SUPPORTO SCRIPT
  if (name == sPluginType_id) {
    NPObject *myobj =
      NPN_CreateObject(mNpp, GET_NPOBJECT_CLASS(BasePlugin));
    if (!myobj) {
      return false;
    }

    OBJECT_TO_NPVARIANT(myobj, *result);

    return true;
  }
  //

  /*if (name == sBar_id) {
    static int a = 17;

    INT32_TO_NPVARIANT(a, *result);

    a += 5;

    return true;
  }*/ 

  return true;
}

/// returns true if the invoked method is managed and executes the appropriate code 
/// filling the NPVariant pointer with data if needed
bool
BasePlugin::Invoke(NPIdentifier name, const NPVariant *args,
                               uint32_t argCount, NPVariant *result)
{
  return false;
}

/// Manages the invocation of the default '()' method
bool
BasePlugin::InvokeDefault(const NPVariant *args, uint32_t argCount,
                                      NPVariant *result)
{
   return false;
}

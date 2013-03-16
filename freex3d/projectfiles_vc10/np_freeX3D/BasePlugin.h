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

#pragma once
#include "ScriptablePluginObjectBase.h"

/// We define the identifiers for the interaction between plugin and page

/// DOM page objects identifiers
static NPIdentifier sDocument_id;			//document object
static NPIdentifier sBody_id;				//body object

// Scripting engine methods identifiers
static NPIdentifier sCreateElement_id;		//create element
static NPIdentifier sCreateTextNode_id;		//create node
static NPIdentifier sAppendChild_id;		//append child

//Plugin specific identifiers
static NPIdentifier sPluginType_id;			//plugin type
static NPIdentifier sFoo_id;				//plugin method "foo"
static NPIdentifier sBar_id;				//plugin property "bar"
static NPIdentifier sShowText_id;			//plugin method "showText"
static NPIdentifier sClearWindow_id;		//plugin method "clearWindow"

static NPObject *sWindowObj;				//pointer to the window object


class BasePlugin : public ScriptablePluginObjectBase
{
public:
  BasePlugin(NPP npp)
    : ScriptablePluginObjectBase(npp)
  {
  }

  virtual bool HasMethod(NPIdentifier name);
  virtual bool HasProperty(NPIdentifier name);
  virtual bool GetProperty(NPIdentifier name, NPVariant *result);
  virtual bool Invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t argCount, NPVariant *result);
  virtual bool InvokeDefault(const NPVariant *args, uint32_t argCount,
                             NPVariant *result);
};

static NPObject *
AllocateScriptablePluginObject(NPP npp, NPClass *aClass)
{
  return new BasePlugin(npp);
}

DECLARE_NPOBJECT_CLASS_WITH_BASE(BasePlugin,
                                 AllocateScriptablePluginObject);

/// Returns true if the NPIdentifier passed is managed as a scriptable method

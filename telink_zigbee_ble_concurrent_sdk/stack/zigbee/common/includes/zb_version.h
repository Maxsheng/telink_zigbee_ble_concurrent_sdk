/********************************************************************************************************
 * @file    zb_version.h
 *
 * @brief   This is the header file for zb_version
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#ifndef ZB_VERSION_H
#define ZB_VERSION_H

#define SDK_VERSION_ID				v2.4.0.0

#define	SDK_VERSION_(sdk_version)	"$$$zb_sdk_"#sdk_version"$$$"
#define	SDK_VERSION(sdk_version)	SDK_VERSION_(sdk_version)

#endif	/* ZB_VERSION_H */

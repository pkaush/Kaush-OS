/*
 *  C Interface:	hal.h
 *
 * Description: This will be internal file for sharing the declarations within HAL
 *
 *
 * Author: Puneet Kaushik <puneet.kaushik@gmail.com>, (C) 2010
 *
 * Copyright: See COPYRIGHT file that comes with this distribution
 *
 */
#include <inc/types.h>

USHORT
Pic8259GetMask();

VOID Pic8259SetMask(USHORT mask);

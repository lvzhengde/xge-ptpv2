/*
 * PTPv2 protocol test application
 */

#ifndef _PTPD_H__
#define _PTPD_H__

#include "MyApp.h"
#include "datatypes.h"

class ptpd: public MyApp
{
  //member methods
  public:
    ptpd(controller *pController);
  
    virtual ~ptpd();
  
    virtual void init();
  
    virtual void exec();
  
    virtual void exit();

  //member variables
  protected:
    PtpClock *G_ptpClock;
};

#endif // _PTPD_H__


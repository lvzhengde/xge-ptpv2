/*
 * base class for the applications running on controller
 */

#ifndef _MY_APP_H__
#define _MY_APP_H__

class controller;

class MyApp
{
  //member methods
  public:
    MyApp(controller *pController)
      : m_pController(pController)
    {};
  
    virtual ~MyApp(){};
  
    virtual void init() = 0;
  
    virtual void exec() = 0;
  
    virtual void exit() = 0;

  //member variables
  protected:
    controller *m_pController;
};

#endif // _MY_APP_H__

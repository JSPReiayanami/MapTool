#ifndef __Interface_Entity_H__
#define __Interface_Entity_H__
#include "Head.h"
class Interface_Entity : public Layout
{
public:
	Interface_Entity();
	~Interface_Entity();
	/*
		@固定创建方法
	*/
	virtual bool init();
	
	/*
		@根节点
	*/
	Layout * m_Root;//根节点
	string   m_LayerSrc;//界面资源
	/*
		@设置自己的名字
	*/
	virtual void SetInterfaceName();
	/*
		@初始化资源
	*/
	virtual void InitSrcLayout();
	/*
		@初始化控件
	*/
	virtual void InitWidget();
	/*
		@初始化按钮回调
	*/
	virtual void InitButtonEvent();
protected:

private:
	

};
#endif // !__MAINSCENE__
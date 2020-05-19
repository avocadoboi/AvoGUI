/*
	This little program was created to help with debugging and building the mouse event system.
	It can also be useful for library users who want to understand the mouse event system better!
*/

#include "../AvoGUI.hpp"

//------------------------------

uint32_t frameCount = 0;

class MouseView : public Avo::View
{
private:
	std::string m_name;

public:
	MouseView(View* p_parent, Avo::Rectangle<float> const& p_bounds) :
		View(p_parent, p_bounds)
	{
		enableMouseEvents();
	}

	//------------------------------

	void setName(char const* p_name)
	{
		m_name = p_name;
	}

	//------------------------------

	void handleMouseDown(Avo::MouseEvent const& p_event) override
	{
		std::cout << m_name + " mouse down" << std::endl;
	}
	void handleMouseUp(Avo::MouseEvent const& p_event) override
	{
		std::cout << m_name + " mouse up" << std::endl;
	}
	// Commented out because this spams the console...
	//void handleMouseMove(Avo::MouseEvent const& p_event) override
	//{
	//	std::cout << m_name + "mouse move" << std::endl;
	//}
	void handleMouseEnter(Avo::MouseEvent const& p_event) override
	{
		std::cout << m_name + " mouse enter" << std::endl;
	}
	void handleMouseLeave(Avo::MouseEvent const& p_event) override
	{
		std::cout << m_name + " mouse leave" << std::endl;
	}
	void handleMouseBackgroundEnter(Avo::MouseEvent const& p_event) override
	{
		std::cout << m_name + " mouse background enter" << std::endl;
	}
	void handleMouseBackgroundLeave(Avo::MouseEvent const& p_event) override
	{
		std::cout << m_name + " mouse background leave" << "\n\n";
		frameCount = 0;
	}

	//------------------------------

	void draw(Avo::DrawingContext* p_context) override
	{
		p_context->setColor(getThemeColor(Avo::ThemeColors::background));
		p_context->fillRectangle(getSize());
	}
};

//------------------------------

class MouseEventsTest : public Avo::Gui
{
public:
	MouseEventsTest()
	{
		create("Mouse events test", 800, 600, Avo::WindowStyleFlags::DefaultNoResize);

		enableMouseEvents();

		MouseView* container_0 = new MouseView(this, { 100, 100, 400, 300 });
		container_0->setName("Red");
		container_0->setThemeColor(Avo::ThemeColors::background, 0xffff0000);

		MouseView* child = new MouseView(container_0, { 10, 10, container_0->getWidth() - 10, container_0->getHeight() - 10 });
		child->setName("Green");
		child->setThemeColor(Avo::ThemeColors::background, 0xff00ff00);

		MouseView* container_1 = new MouseView(this, { 300, 200, 500, 400 });
		container_1->setName("Yellow");
		container_1->setThemeColor(Avo::ThemeColors::background, 0xffffff00);

		child = new MouseView(container_1, { 70, 50, 150, 120 });
		child->setName("Cyan");
		child->setThemeColor(Avo::ThemeColors::background, 0xff00ffff);

		child = new MouseView(container_1, { 100, 100, 180, 180 });
		child->setName("Blue");
		child->setThemeColor(Avo::ThemeColors::background, 0xff0000ff);

		MouseView* overlay = new MouseView(this, { 0, 0, getWidth(), getHeight() });
		overlay->setName("Overlay");
		overlay->setThemeColor(Avo::ThemeColors::background, 0U);
		overlay->setIsOverlay(true);

		queueAnimationUpdate();

		run();
	}

	//------------------------------

	void handleMouseDown(Avo::MouseEvent const& p_event) override
	{
		std::cout << "GUI mouse down" << std::endl;
	}
	void handleMouseUp(Avo::MouseEvent const& p_event) override
	{
		std::cout << "GUI mouse up" << std::endl;
	}
	void handleMouseMove(Avo::MouseEvent const& p_event) override
	{
		invalidate();
	}
	void handleMouseEnter(Avo::MouseEvent const& p_event) override
	{
		std::cout << "GUI mouse enter" << std::endl;
	}
	void handleMouseLeave(Avo::MouseEvent const& p_event) override
	{
		std::cout << "GUI mouse leave" << std::endl;
	}
	void handleMouseBackgroundEnter(Avo::MouseEvent const& p_event) override
	{
		std::cout << "GUI mouse background enter" << std::endl;
	}
	void handleMouseBackgroundLeave(Avo::MouseEvent const& p_event) override
	{
		std::cout << "GUI mouse background leave" << "\n\n";
		frameCount = 0;
	}

	//------------------------------

	void updateAnimations()
	{
		if (frameCount == 15 * 60)
		{
			std::system("cls");
		}
		frameCount++;
		queueAnimationUpdate();
	}
};

//------------------------------

int main()
{
	new MouseEventsTest();
}

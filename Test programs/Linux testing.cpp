#include <AvoGUI.hpp>

//------------------------------

class App :
	public AvoGUI::Gui
{
private:
	float m_time;

public:
	App() :
		m_time(0.f)
	{
		create(u8"Linux test! Unicode: åäöâñëV݉sZ㇡ه搶o7賍", 600, 500, AvoGUI::WindowStyleFlags::Default);
		waitForFinish();
	}

	void createContent() override
	{
		setThemeColor("background", AvoGUI::Color(0));

		queueAnimationUpdate();
	}
	void updateAnimations() override
	{
		m_time += 0.005f;

		invalidate();
		queueAnimationUpdate();
	}

	void draw(AvoGUI::DrawingContext *p_context) override
	{
		p_context->setColor(AvoGUI::Color(1.f, 0.1f, 0.5f));
		for (uint32 a = 0; a < 10; a++)
		{
			p_context->drawLine(10.f, 10.5f + a*10.f, 100.f, 50.5f + a*10.f, 0.5f + a*0.5f);
		}

		p_context->setColor(AvoGUI::Color(0.2f, 0.5f, 0.3f));
		p_context->fillRectangle(getCenterX() - 80.f, 10.f, getCenterX() + 80.f, 120.f, AvoGUI::RectangleCorners(10.f, AvoGUI::RectangleCornerType::Round));
		p_context->fillRectangle(getCenterX() - 80.f, getBottom() - 120.f, getCenterX() + 80.f, getBottom() - 10.f, AvoGUI::RectangleCorners(10.f, AvoGUI::RectangleCornerType::Cut));

		p_context->setColor(AvoGUI::Color(0.1f, 1.f, 0.5f));
		p_context->rotate(-m_time, getCenter());
		p_context->strokeRectangle(AvoGUI::Rectangle(-50.f, -30.f, 50.f, 30.f) += getCenter(), 2.f);
		p_context->fillRectangle(AvoGUI::Rectangle(-40.f, -20.f, 40.f, 20.f) += getCenter());
		p_context->rotate(m_time, getCenter());

		p_context->setColor(AvoGUI::Color(0.8f, 0.8f, 0.1f));
		p_context->strokeCircle(AvoGUI::Point<float>().setPolar(m_time, 100.f) += getCenter(), std::sin(m_time)*30.f + 30.f, 2.f);
		p_context->fillCircle(AvoGUI::Point<float>().setPolar(m_time + AvoGUI::PI, 100.f) += getCenter(), std::sin(m_time + AvoGUI::PI)*30.f + 30.f);
	}
};

//------------------------------

int main()
{
	new App();
}
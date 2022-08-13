#ifndef BRR_WINDOW_H
#define BRR_WINDOW_H

namespace brr
{
	class Window
	{
	public:
		~Window();

		// Initialize and open the window with the current properties.
		uint32_t InitWindow();

		// Close the window if it is open. If it is already closed, then do nothing.
		void CloseWindow();

		// Process a window event
		void ProcessWindowEvent(const SDL_WindowEvent& pWindowEvent);

		void GetRequiredVulkanExtensions(std::vector<const char*>& extensions) const;
		[[nodiscard]] glm::ivec2 GetWindowExtent() const;
		[[nodiscard]] vk::SurfaceKHR GetVulkanSurface(vk::Instance instance) const;

		[[nodiscard]] uint32_t GetWindowID() const { return m_pWindowID; }
		[[nodiscard]] SDL_Window* GetSDLWindowHandle() const { return m_pWindow; }
		[[nodiscard]] bool NeedToClose() const { return m_pNeedToClose; }

	private:
		bool m_pNeedToClose = false;
		uint32_t m_pWindowID = 0;
		SDL_Window* m_pWindow = nullptr;
	};
}

#endif
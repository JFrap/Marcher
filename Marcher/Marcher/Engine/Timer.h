#pragma once

#include <chrono>

namespace marcher {
	class Timer {
	public:
		Timer() {
			m_lastTime = std::chrono::steady_clock::now();
		}

		template<class T>
		T CurrentTime() {
			auto current = std::chrono::steady_clock::now();
			T totTime = std::chrono::duration<T>(current - m_lastTime).count();
			return totTime;
		}

		template<class T>
		T Restart() {
			auto current = std::chrono::steady_clock::now();
			T totTime = std::chrono::duration<T>(current - m_lastTime).count();
			m_lastTime = current;
			return totTime;
		}

		void Restart() {
			m_lastTime = std::chrono::steady_clock::now();
		}

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_lastTime; //The time restart() will be relative to.
	};
}

#pragma once

#include <cstdint>
#include <deque>
#include <chrono>
#include <algorithm>

namespace utils
{
class SlidingWindowRealTime {
  public:
    explicit SlidingWindowRealTime(size_t duration)
    {
        _capacity = duration;
    }

    SlidingWindowRealTime() = delete;
    SlidingWindowRealTime(SlidingWindowRealTime &&rhs) = delete;
    SlidingWindowRealTime(SlidingWindowRealTime &rhs) = delete;

    void append()
    {
        auto now = std::chrono::steady_clock::now();
        _data.push_back(now);
    }

    float fps()
    {
        float v = 0.0f;

        shift();

        if (_data.size() < 2)
        {
            return v;
        }

        auto &b = _data.front();
        auto &e = _data.back();
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - b).count();
        if (d > 0)
        {
            v = (_data.size() * 1000.0f) / d;
        }

        return v;
    }

  private:
    void shift()
    {
        auto now = std::chrono::steady_clock::now();
        while (!_data.empty())
        {
            auto &b = _data.front();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(now - b).count();
            if (d < _capacity)
            {
                break;
            }
            _data.pop_front();
        }
    }

  private:
    std::deque<std::chrono::steady_clock::time_point> _data;
    int64_t _capacity = 5000;
};

class SlidingWindow {
  public:
    explicit SlidingWindow(size_t duration)
    {
        _capacity = duration;
    }

    SlidingWindow() = delete;
    SlidingWindow(SlidingWindow &&rhs) = delete;
    SlidingWindow(SlidingWindow &rhs) = delete;

    void append()
    {
        _data.push_back(std::chrono::steady_clock::now());
        while (!_data.empty())
        {
            auto &b = _data.front();
            auto &e = _data.back();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - b).count();
            if (d < _capacity)
            {
                break;
            }
            _data.pop_front();
        }
    }

    float fps()
    {
        float v = 0.0f;
        if (_data.size() < 2)
        {
            return v;
        }

        auto &b = _data.front();
        auto &e = _data.back();
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - b).count();
        if (d > 0)
        {
            v = ((_data.size() - 1) * 1000.0f) / d;
        }
        return v;
    }

    size_t size()
    {
        return _data.size();
    }

  private:
    std::deque<std::chrono::steady_clock::time_point> _data;
    int64_t _capacity = 5000;
};

class DurationSlidingWindow {
  public:
    explicit DurationSlidingWindow(size_t duration)
    {
        _capacity = duration;
    }

    DurationSlidingWindow() = delete;
    DurationSlidingWindow(DurationSlidingWindow &&rhs) = delete;
    DurationSlidingWindow(DurationSlidingWindow &rhs) = delete;

    void append()
    {
        _data.push_back(std::chrono::steady_clock::now());
        if (_data.size() == 1)
        {
            _duration.push_back(0);
        }
        else
        {
            auto b = _data.end() - 2;
            auto &e = _data.back();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - *b).count();
            _duration.push_back(d);
        }

        while (!_data.empty())
        {
            auto &b = _data.front();
            auto &e = _data.back();
            auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - b).count();
            if (d < _capacity)
            {
                break;
            }
            _data.pop_front();
            _duration.pop_front();
        }
    }

    float fps()
    {
        float v = 0.0f;
        if (_data.size() < 2)
        {
            return v;
        }

        auto &b = _data.front();
        auto &e = _data.back();
        auto d = std::chrono::duration_cast<std::chrono::milliseconds>(e - b).count();
        v = ((_data.size() - 1) * 1000.0f) / d;
        return v;
    }

    void minMax(uint64_t &min, uint64_t &max)
    {
        if (_data.empty())
        {
            return;
        }
        std::deque<int64_t> t = _duration;
        std::sort(t.begin(), t.end());
        min = t.front();
        max = t.back();
    }

  private:
    std::deque<std::chrono::steady_clock::time_point> _data;
    std::deque<int64_t> _duration;
    int64_t _capacity = 5000;
};

} // namespace utils
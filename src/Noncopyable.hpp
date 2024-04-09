#ifndef MYCOROUTINE_NONCOPYABLE_HPP
#define MYCOROUTINE_NONCOPYABLE_HPP
namespace myCoroutine {
class Noncopyable {
public:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};
}
#endif // MYCOROUTINE_NONCOPYABLE_HPP
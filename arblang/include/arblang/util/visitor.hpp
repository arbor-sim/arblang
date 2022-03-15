#pragma once

// helper type for std::visit
namespace al{
namespace util{
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}
} // namespace al

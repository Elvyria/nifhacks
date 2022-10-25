#include <fmt/format.h>

template <> struct fmt::formatter<Vector3> {
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
		return ctx.begin();
	}

	template <typename FormatContext>
	auto format(const Vector3& v, FormatContext& ctx) const -> decltype(ctx.out()) {
		return format_to(ctx.out(), "{} {} {}", v.x, v.y, v.z);
	}
};

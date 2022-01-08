#pragma once

#include <tuple>
#include <variant>
#include <string_view>

#define Forward(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

template<typename UnknownType, typename ReferenceType>
concept SubtypeOf = std::same_as<std::decay_t<UnknownType>, ReferenceType> || std::derived_from<std::decay_t<UnknownType>, ReferenceType>;

template<typename UnknownType, typename ...ReferenceTypes>
concept AnyOf = (SubtypeOf<UnknownType, ReferenceTypes> || ...);

template<typename UnknownType, typename ...ReferenceTypes>
concept AnyBut = !AnyOf<UnknownType, ReferenceTypes...>;

template<typename UnknownType, typename ReferenceType>
concept ExplicitlyConvertibleTo = requires(UnknownType x) { static_cast<ReferenceType>(Forward(x)); };

template<typename UnknownType, typename ReferenceType>
concept ConstructibleFrom = ExplicitlyConvertibleTo<ReferenceType, std::decay_t<UnknownType>>;

template<typename UnknownType>
concept BuiltinArray = std::is_array_v<std::remove_cvref_t<UnknownType>>;

template<typename UnknownType>
concept Advanceable = requires(UnknownType x) { ++x; };

template<typename UnknownType>  //�ɵ�������Լ����������ͨ����������ṩ���µ����ӿڵĶ���
concept Iterable = BuiltinArray<UnknownType> || requires(UnknownType x) {
	{ x.begin() }->Advanceable;
	{ *x.begin() }->AnyBut<void>;
	{ x.begin() != x.end() }->ExplicitlyConvertibleTo<bool>;
};

// ���޸���������Ķ��壺10 ��
// ֧�ִ�ӡһ��Ŀɵ�������
auto& operator<<(SubtypeOf<std::ostream> auto& printer, Iterable auto&& container) requires (
	(requires { printer << *container.begin(); } ||    //Ҫ�� printer �� begin() �ӿڼ���
		requires { printer << container[0]; }) && !ConstructibleFrom<decltype(container), const char*>)  //���� printer ������Ԫ�ؼ��ݣ������������ַ�����ʹ�ø����أ�
{
	auto [Startpoint, Endpoint] = [&] {
		if constexpr (BuiltinArray<decltype(container)>) {
			return std::tuple{ container, container + sizeof(container) / sizeof(container[0]) };
		}
		else {
			return std::tuple{ container.begin(), container.end() };
		}
	}();
	std::string_view separator = "";
	printer << "{";
	for (; Startpoint != Endpoint; ++Startpoint) {
		printer << std::exchange(separator, ", ") << *Startpoint;
	}
	printer << "}";
	return printer;
}

//֧�ִ�ӡ�ɵ��������variant
template <class ...Iterable> requires (sizeof...(Iterable) > 0)
auto& operator<<(SubtypeOf<std::ostream> auto& printer, std::variant<Iterable...> const& var)
	requires ((requires { (printer << std::get<Iterable>(var)); }) && ...)
{
	std::visit([&](auto&& arg) { printer << arg; }, var);
	return printer;
	// ��ʵ���Զ�ƥ�������о������͵Ĵ�ӡ��10 ��
}
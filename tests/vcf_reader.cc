/*
 * Copyright (c) 2020-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <libbio/vcf/variant_printer.hh>
#include <libbio/vcf/vcf_reader.hh>
#include <map>
#include <range/v3/all.hpp>
#include <set>
#include <string>
#include <typeinfo>

namespace gen	= Catch::Generators;
namespace io	= boost::iostreams;
namespace lb	= libbio;
namespace rsv	= ranges::view;
namespace vcf	= libbio::vcf;


namespace {
	
	template <typename t_range>
	auto from_range(t_range &&range)
	{
		return Catch::Generators::from_range(ranges::begin(range), ranges::end(range));
	}
	
	
	template <typename t_key, typename t_value, typename t_cmp>
	void add_map_keys(std::map <t_key, t_value, t_cmp> const &map, std::vector <t_key> &dst)
	{
		for (auto const &[key, value] : map)
			dst.push_back(key);
	}


	template <typename t_value>
	void add_to_sorted_vector(std::vector <t_value> const &src, std::vector <t_value> &dst)
	{
		auto const original_size(dst.size());
		dst.insert(dst.end(), src.begin(), src.end());
		std::inplace_merge(dst.begin(), dst.begin() + original_size, dst.end());
	}
	
	
	template <typename t_key, typename t_value, typename t_cmp>
	std::vector <t_key> map_keys(std::map <t_key, t_value, t_cmp> const &map)
	{
		std::vector <t_key> retval;
		add_map_keys(map, retval);
		return retval;
	}
	
	
	struct metadata_description
	{
		std::int32_t					number{};
		vcf::metadata_value_type	value_type{};
		
		metadata_description() = default;
		
		metadata_description(std::int32_t number_, vcf::metadata_value_type value_type_):
			number(number_),
			value_type(value_type_)
		{
		}
	};
	
	typedef std::map <std::string, metadata_description>	metadata_map;
	
	
	template <typename t_actual_fields>
	void check_metadata_fields(
		metadata_map const &expected_fields,
		t_actual_fields const &actual_fields,
		bool const should_have_metadata
	)
	{
		REQUIRE(actual_fields.size() == expected_fields.size());
		
		for (auto const &[id, expected] : expected_fields)
		{
			auto const it(actual_fields.find(id));
			REQUIRE(it != actual_fields.end());
			auto const &actual_field_ptr(it->second);
			REQUIRE(actual_field_ptr.get());
			auto const &actual_field(*actual_field_ptr);
			
			auto const actual_number(actual_field.number());
			REQUIRE((
					actual_number == expected.number ||
					(actual_field.value_type_is_vector() && actual_number == vcf::VCF_NUMBER_DETERMINED_AT_RUNTIME)
			));
			REQUIRE(actual_field.metadata_value_type() == expected.value_type);
			
			// Predefined fields do not have metadata unless declared in VCF headers.
			if (should_have_metadata)
			{
				auto const *meta(actual_field.get_metadata());
				REQUIRE(meta);
				REQUIRE(meta->get_number() == expected.number);
				REQUIRE(meta->get_value_type() == expected.value_type);
			}
		}
	}
	
	
	struct gt_field
	{
		std::uint16_t	lhs{};
		std::uint16_t	rhs{};
		bool			is_phased{};
		bool			is_in_record{};
		
		gt_field() = default;
		
		gt_field(std::uint16_t lhs_, std::uint16_t rhs_, bool is_phased_):
			lhs(lhs_),
			rhs(rhs_),
			is_phased(is_phased_),
			is_in_record(true)
		{
		}
	};
	
	
	template <vcf::metadata_value_type t_value_type>
	struct value_type_mapping {};
	
	template <> struct value_type_mapping <vcf::metadata_value_type::INTEGER>	{ typedef std::int32_t	type; };
	template <> struct value_type_mapping <vcf::metadata_value_type::FLOAT>		{ typedef float			type; };
	template <> struct value_type_mapping <vcf::metadata_value_type::CHARACTER>	{ typedef std::string	type; };
	template <> struct value_type_mapping <vcf::metadata_value_type::STRING>	{ typedef std::string	type; };
	
	template <vcf::metadata_value_type t_value_type>
	using value_type_mapping_t = typename value_type_mapping <t_value_type>::type;
	
	template <vcf::metadata_value_type t_value_type>
	struct constant_from_value_type
	{
		typedef std::integral_constant <vcf::metadata_value_type, t_value_type> type;
	};
		
	template <vcf::metadata_value_type t_value_type>
	inline constexpr auto constant_from_value_type_v = typename constant_from_value_type <t_value_type>::type{};
	
	
	template <typename t_value>
	struct is_vector_type
	{
		typedef std::false_type	type;
	};
	
	template <typename t_value>
	struct is_vector_type <std::vector <t_value>>
	{
		typedef std::true_type	type;
	};
	
	template <typename t_value>
	inline constexpr bool is_vector_type_v = is_vector_type <t_value>::type::value;
	
	
	template <typename t_cb>
	void visit_value_type(vcf::metadata_value_type value_type, bool is_vector, t_cb &&cb)
	{
		if (is_vector)
		{
			switch (value_type)
			{
				case vcf::metadata_value_type::INTEGER:
					cb(constant_from_value_type_v <vcf::metadata_value_type::INTEGER>, std::true_type());
					break;
				case vcf::metadata_value_type::FLOAT:
					cb(constant_from_value_type_v <vcf::metadata_value_type::FLOAT>, std::true_type());
					break;
				case vcf::metadata_value_type::CHARACTER:
					cb(constant_from_value_type_v <vcf::metadata_value_type::CHARACTER>, std::true_type());
					break;
				case vcf::metadata_value_type::STRING:
					cb(constant_from_value_type_v <vcf::metadata_value_type::STRING>, std::true_type());
					break;
				default:
					throw std::runtime_error("Unexpected value type");
			}
		}
		else
		{
			switch (value_type)
			{
				case vcf::metadata_value_type::INTEGER:
					cb(constant_from_value_type_v <vcf::metadata_value_type::INTEGER>, std::false_type());
					break;
				case vcf::metadata_value_type::FLOAT:
					cb(constant_from_value_type_v <vcf::metadata_value_type::FLOAT>, std::false_type());
					break;
				case vcf::metadata_value_type::CHARACTER:
					cb(constant_from_value_type_v <vcf::metadata_value_type::CHARACTER>, std::false_type());
					break;
				case vcf::metadata_value_type::STRING:
					cb(constant_from_value_type_v <vcf::metadata_value_type::STRING>, std::false_type());
					break;
				default:
					throw std::runtime_error("Unexpected value type");
			}
		}
	}
	
	
	template <typename t_type>
	struct process_value
	{
		constexpr t_type const &process(t_type const &value) const { return value; }
	};
	
	template <>
	struct process_value <std::string_view>
	{
		std::string process(std::string_view const &value) const { return std::string(value); }
	};
	
	template <>
	struct process_value <std::vector <std::string_view>>
	{
		std::vector <std::string> process(std::vector <std::string_view> const &vec) const
		{
			return std::vector <std::string>(vec.begin(), vec.end());
		}
	};
	
	
	struct record_value
	{
		typedef std::variant <
			std::int32_t,
			float,
			std::string,
			std::vector <std::int32_t>,
			std::vector <float>,
			std::vector <std::string>
		>								value_variant;
		
		value_variant					value;
		
		template <typename t_type>
		record_value(vcf::metadata_value_type value_type_, bool is_vector_, t_type const &value_)
		{
			visit_value_type(value_type_, is_vector_, [this, &value_](auto const value_type, auto const is_vector){
				if constexpr (is_vector() == is_vector_type_v <t_type>)
				{
					constexpr auto const vt{value_type()}; // Passing this directly to value_type_mapping_t causes an internal compiler error on GCC 13.2.0.
					typedef value_type_mapping_t <vt> scalar_value_type;
					typedef std::conditional_t <is_vector(), std::vector <scalar_value_type>, scalar_value_type> chosen_value_type;

					if constexpr (std::is_same_v <t_type, chosen_value_type>)
						value.emplace <chosen_value_type>(value_);
					else
						throw std::runtime_error("Given value does not match the specified value type");
				}
				else
				{
					throw std::runtime_error("Scalarity of the given value does not match the specified value type");
				}
			});
		}
	};
	
	
	struct expected_record
	{
		std::uint64_t								lineno{};
		std::uint64_t								pos{};
		std::string									id{};
		std::string									ref{};
		std::vector <std::string>					alts{};
		gt_field									gt{};
		std::set <std::string>						empty_info_ids{};
		std::set <std::string>						empty_genotype_ids{};
		std::set <std::string>						unset_genotype_ids{};
		std::set <std::string>						set_flags{};
		std::set <std::string>						unset_flags{};
		std::map <std::string, record_value>		info_values{};
		std::map <std::string, record_value>		genotype_values{};
		
		expected_record(
			std::uint64_t							lineno_,
			std::uint64_t							pos_,
			char const								*id_,
			char const								*ref_,
			std::initializer_list <char const *>	alts_,
			gt_field								gt_,
			std::initializer_list <char const *>	set_flags_,
			std::initializer_list <char const *>	unset_flags_
		):
			lineno(lineno_),
			pos(pos_),
			id(id_),
			ref(ref_),
			alts(alts_.begin(), alts_.end()),
			gt(gt_),
			set_flags(set_flags_.begin(), set_flags_.end()),
			unset_flags(unset_flags_.begin(), unset_flags_.end())
		{
		}
		
		template <typename t_type>
		void add_info_value(char const *id, metadata_map const &metadata, t_type &&value)
		{
			auto const it(metadata.find(id));
			REQUIRE(metadata.end() != it);
			auto const is_vector(vcf::value_count_corresponds_to_vector(it->second.number));
			info_values.emplace(id, record_value(it->second.value_type, is_vector, std::forward <t_type>(value)));
		}
		
		template <typename t_type>
		void add_genotype_value(char const *id, metadata_map const &metadata, t_type &&value)
		{
			auto const it(metadata.find(id));
			REQUIRE(metadata.end() != it);
			auto const is_vector(vcf::value_count_corresponds_to_vector(it->second.number));
			genotype_values.emplace(id, record_value(it->second.value_type, is_vector, std::forward <t_type>(value)));
		}
		
		template <typename t_string>
		void add_empty_info_id(t_string const &id)
		{
			empty_info_ids.emplace(id);
		}
		
		template <typename t_string>
		void add_empty_genotype_id(t_string const &id)
		{
			empty_genotype_ids.emplace(id);
		}
		
		template <typename t_string>
		void add_unset_genotype_id(t_string const &id)
		{
			unset_genotype_ids.emplace(id);
		}
	};


	template <typename t_container, typename t_field, typename t_field_values>
	void check_expected_field(
		t_container const &ct,
		t_field const &field,
		t_field_values const &field_values,
		std::set <std::string> const empty_field_ids
	)
	{
		auto const &id(field.get_metadata()->get_id());
		auto const nonempty_it(field_values.find(id));
		auto const empty_it(empty_field_ids.find(id));
		
		{
			bool const found_in_empty(empty_field_ids.end() != empty_it);
			bool const found_in_nonempty(field_values.end() != nonempty_it);
			// Operator ^ not available for use in REQUIRE.
			REQUIRE((found_in_empty || found_in_nonempty));
			REQUIRE(!(found_in_empty && found_in_nonempty));
		}
	
		if (field_values.end() != nonempty_it)
		{
			REQUIRE(field.has_value(ct));
			auto const &expected_value(nonempty_it->second);
		
			REQUIRE_NOTHROW([&field, &ct, &expected_value](){
				visit_value_type(
					field.metadata_value_type(),
					field.value_type_is_vector(),
					[&field, &ct, &expected_value](auto const value_type, auto const is_vector){
						typedef typename t_field::template typed_field_type <value_type(), is_vector()> typed_field_type;
						
						auto const &typed_field(dynamic_cast <typed_field_type const &>(field));
						auto const &unprocessed_value(typed_field(ct));
						
						typedef std::remove_cvref_t <decltype(unprocessed_value)> unprocessed_value_type;
						process_value <unprocessed_value_type> const process;
						auto const &actual_value(process.process(unprocessed_value));
						
						typedef std::remove_cvref_t <decltype(actual_value)> actual_value_type;
	 					REQUIRE(std::holds_alternative <actual_value_type>(expected_value.value));
						REQUIRE(std::get <actual_value_type>(expected_value.value) == actual_value);
					}
				);
			});
		}
		else
		{
			REQUIRE(!field.has_value(ct));
		}
	}
	
	
	static metadata_map const s_type_test_expected_info_fields{
		{"INFO_FLAG",			{0,							vcf::metadata_value_type::FLAG}},
		{"INFO_FLAG_2",			{0,							vcf::metadata_value_type::FLAG}},
		{"INFO_INTEGER",		{1,							vcf::metadata_value_type::INTEGER}},
		{"INFO_FLOAT",			{1,							vcf::metadata_value_type::FLOAT}},
		{"INFO_CHARACTER",		{1,							vcf::metadata_value_type::CHARACTER}},
		{"INFO_STRING",			{1,							vcf::metadata_value_type::STRING}},
		{"INFO_INTEGER_4",		{4,							vcf::metadata_value_type::INTEGER}},
		{"INFO_FLOAT_4",		{4,							vcf::metadata_value_type::FLOAT}},
		{"INFO_CHARACTER_4",	{4,							vcf::metadata_value_type::CHARACTER}},
		{"INFO_STRING_4",		{4,							vcf::metadata_value_type::STRING}},
		{"INFO_INTEGER_A",		{vcf::VCF_NUMBER_A,			vcf::metadata_value_type::INTEGER}},
		{"INFO_INTEGER_R",		{vcf::VCF_NUMBER_R,			vcf::metadata_value_type::INTEGER}},
		{"INFO_INTEGER_G",		{vcf::VCF_NUMBER_G,			vcf::metadata_value_type::INTEGER}},
		{"INFO_INTEGER_D",		{vcf::VCF_NUMBER_UNKNOWN,	vcf::metadata_value_type::INTEGER}}
	};
	
	static metadata_map const s_type_test_expected_genotype_fields{ // GT not included.
		{"FORMAT_INTEGER",		{1,							vcf::metadata_value_type::INTEGER}},
		{"FORMAT_FLOAT",		{1,							vcf::metadata_value_type::FLOAT}},
		{"FORMAT_CHARACTER",	{1,							vcf::metadata_value_type::CHARACTER}},
		{"FORMAT_STRING",		{1,							vcf::metadata_value_type::STRING}},
		{"FORMAT_INTEGER_4",	{4,							vcf::metadata_value_type::INTEGER}},
		{"FORMAT_FLOAT_4",		{4,							vcf::metadata_value_type::FLOAT}},
		{"FORMAT_CHARACTER_4",	{4,							vcf::metadata_value_type::CHARACTER}},
		{"FORMAT_STRING_4",		{4,							vcf::metadata_value_type::STRING}},
		{"FORMAT_INTEGER_A",	{vcf::VCF_NUMBER_A,			vcf::metadata_value_type::INTEGER}},
		{"FORMAT_INTEGER_R",	{vcf::VCF_NUMBER_R,			vcf::metadata_value_type::INTEGER}},
		{"FORMAT_INTEGER_G",	{vcf::VCF_NUMBER_G,			vcf::metadata_value_type::INTEGER}},
		{"FORMAT_INTEGER_D",	{vcf::VCF_NUMBER_UNKNOWN,	vcf::metadata_value_type::INTEGER}}
	};
	
	
	std::vector <expected_record> prepare_expected_records_for_test_simple_vcf()
	{
		std::vector <expected_record> expected_records{
			{3,	8,	"a",		"C",	{"G"},		{},	{},	{}},
			{4,	10,	"b",		"A",	{"C", "G"},	{},	{},	{}}
		};
		
		return expected_records;
	}
	
	
	std::vector <expected_record> prepare_expected_records_for_test_data_types_vcf()
	{
		std::vector <expected_record> expected_records{
			{30,	8,	"test_gt_only",		"C",	{"G"},		{1, 1, false},									{},					{"INFO_FLAG", "INFO_FLAG_2"}},
			{31,	10,	"test_most",		"A",	{"C", "G"},	{},												{"INFO_FLAG"},		{"INFO_FLAG_2"}},
			{32,	12,	"test_most_2",		"C",	{"G", "T"},	{},												{"INFO_FLAG_2"},	{"INFO_FLAG"}},
			{33,	14,	"test_gt_only_2",	"G",	{"C"},		{1, 1, true},									{},					{"INFO_FLAG", "INFO_FLAG_2"}},
			{34,	16,	"test_missing",		"T",	{"A"},		{0, vcf::sample_genotype::NULL_ALLELE, false},	{},					{"INFO_FLAG", "INFO_FLAG_2"}}
		};
		
		for (auto const idx : lb::make_array <std::size_t>(0u, 3u, 4u))
		{
			for (auto const &[id, meta] : s_type_test_expected_info_fields)
				expected_records[idx].add_empty_info_id(id);
		}
		
		for (auto const &id : lb::make_array <char const *>(
			"INFO_INTEGER_4",
			"INFO_FLOAT_4",
			"INFO_CHARACTER_4",
			"INFO_STRING_4"
		))
		{
			for (auto &rec : expected_records)
				rec.add_empty_info_id(id);
		}
		
		for (auto const idx : lb::make_array <std::size_t>(0u, 3u))
		{
			for (auto const &[id, meta] : s_type_test_expected_genotype_fields)
				expected_records[idx].add_unset_genotype_id(id);
		}
		
		for (auto const &id : lb::make_array <char const *>(
			"GT",
			"FORMAT_INTEGER",
			"FORMAT_FLOAT",
			"FORMAT_CHARACTER",
			"FORMAT_STRING",
			"FORMAT_INTEGER_A",
			"FORMAT_INTEGER_R",
			"FORMAT_INTEGER_G",
			"FORMAT_INTEGER_D"
		))
		{
			expected_records[4].add_empty_genotype_id(id);
		}
		
		for (auto const &id : lb::make_array <char const *>(
			"FORMAT_INTEGER_4",
			"FORMAT_FLOAT_4",
			"FORMAT_CHARACTER_4",
			"FORMAT_STRING_4"
		))
		{
			for (auto &rec : expected_records)
				rec.add_unset_genotype_id(id);
		}
		
		expected_records[1].add_info_value("INFO_INTEGER",			s_type_test_expected_info_fields,		5);
		expected_records[2].add_info_value("INFO_INTEGER",			s_type_test_expected_info_fields,		7);
		expected_records[1].add_info_value("INFO_FLOAT",			s_type_test_expected_info_fields,		1.025f);
		expected_records[2].add_info_value("INFO_FLOAT",			s_type_test_expected_info_fields,		5.25f);
		expected_records[1].add_info_value("INFO_CHARACTER",		s_type_test_expected_info_fields,		std::string("c"));
		expected_records[2].add_info_value("INFO_CHARACTER",		s_type_test_expected_info_fields,		std::string("e"));
		expected_records[1].add_info_value("INFO_STRING",			s_type_test_expected_info_fields,		std::string("info_test"));
		expected_records[2].add_info_value("INFO_STRING",			s_type_test_expected_info_fields,		std::string("test3"));
		expected_records[1].add_info_value("INFO_INTEGER_A",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({1, 4}));
		expected_records[2].add_info_value("INFO_INTEGER_A",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({3, 6}));
		expected_records[1].add_info_value("INFO_INTEGER_R",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({1, 3, 5}));
		expected_records[2].add_info_value("INFO_INTEGER_R",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({3, 5, 7}));
		expected_records[1].add_info_value("INFO_INTEGER_G",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({3, 7}));
		expected_records[2].add_info_value("INFO_INTEGER_G",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({5, 9}));
		expected_records[1].add_info_value("INFO_INTEGER_D",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({5, 6, 7, 8}));
		expected_records[2].add_info_value("INFO_INTEGER_D",		s_type_test_expected_info_fields,		std::vector <std::int32_t>({7, 8, 9, 10}));
		
		expected_records[1].add_genotype_value("FORMAT_INTEGER",	s_type_test_expected_genotype_fields,	6);
		expected_records[2].add_genotype_value("FORMAT_INTEGER",	s_type_test_expected_genotype_fields,	8);
		expected_records[1].add_genotype_value("FORMAT_FLOAT",		s_type_test_expected_genotype_fields,	2.5f);
		expected_records[2].add_genotype_value("FORMAT_FLOAT",		s_type_test_expected_genotype_fields,	7.75f);
		expected_records[1].add_genotype_value("FORMAT_CHARACTER",	s_type_test_expected_genotype_fields,	std::string("d"));
		expected_records[2].add_genotype_value("FORMAT_CHARACTER",	s_type_test_expected_genotype_fields,	std::string("f"));
		expected_records[1].add_genotype_value("FORMAT_STRING",		s_type_test_expected_genotype_fields,	std::string("sample_test"));
		expected_records[2].add_genotype_value("FORMAT_STRING",		s_type_test_expected_genotype_fields,	std::string("test4"));
		expected_records[1].add_genotype_value("FORMAT_INTEGER_A",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({2, 5}));
		expected_records[2].add_genotype_value("FORMAT_INTEGER_A",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({4, 7}));
		expected_records[1].add_genotype_value("FORMAT_INTEGER_R",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({2, 4, 6}));
		expected_records[2].add_genotype_value("FORMAT_INTEGER_R",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({4, 6, 8}));
		expected_records[1].add_genotype_value("FORMAT_INTEGER_G",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({4, 8}));
		expected_records[2].add_genotype_value("FORMAT_INTEGER_G",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({6, 10}));
		expected_records[1].add_genotype_value("FORMAT_INTEGER_D",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({6, 7, 8, 9}));
		expected_records[2].add_genotype_value("FORMAT_INTEGER_D",	s_type_test_expected_genotype_fields,	std::vector <int32_t>({8, 9, 10, 11}));
		
		return expected_records;
	}
	
	
	template <typename t_variant>
	void check_record_against_expected_in_test_simple_vcf(
		t_variant const &var,
		expected_record const &expected
	)
	{
		REQUIRE(var.chrom_id() == "chr1");
		REQUIRE(var.lineno() == expected.lineno);
		REQUIRE(var.pos() == expected.pos);
		REQUIRE(var.id().size() == 1);
		REQUIRE(var.id().front() == expected.id);
		REQUIRE(var.ref() == expected.ref);
		REQUIRE(var.alts().size() == expected.alts.size());
	}
	
	
	template <typename t_variant, typename t_info_fields, typename t_genotype_fields>
	void check_record_against_expected_in_test_data_types_vcf(
		t_variant const &var,
		expected_record const &expected,
		t_info_fields const &actual_info_fields,
		t_genotype_fields const &actual_genotype_fields
	)
	{
		REQUIRE(var.chrom_id() == "chr1");
		REQUIRE(var.lineno() == expected.lineno);
		REQUIRE(var.pos() == expected.pos);
		REQUIRE(var.id().size() == 1);
		REQUIRE(var.id().front() == expected.id);
		REQUIRE(var.ref() == expected.ref);
		REQUIRE(var.alts().size() == expected.alts.size());
		for (auto const &[actual_alt, expected_alt] : rsv::zip(var.alts(), expected.alts))
		{
			REQUIRE(actual_alt.alt_sv_type == vcf::sv_type::NONE);
			REQUIRE(actual_alt.alt == expected_alt);
		}
	
		// Info fields.
		for (auto const &[id, expected_meta] : s_type_test_expected_info_fields)
		{
			auto const field_it(actual_info_fields.find(id));
			REQUIRE(actual_info_fields.end() != field_it);
		
			if (vcf::metadata_value_type::FLAG == expected_meta.value_type)
			{
				REQUIRE((
					(expected.set_flags.contains(id) && field_it->second->has_value(var)) ||
					(expected.unset_flags.contains(id) && !field_it->second->has_value(var))
				));
			}
			else
			{
				check_expected_field(var, *field_it->second, expected.info_values, expected.empty_info_ids);
			}
		}
	
		// Genotype fields.
		REQUIRE(1 == var.samples().size());
		auto const &first_sample(var.samples().front());
		auto const &genotype_fields_by_id(var.get_format().fields_by_identifier());
		for (auto const &[id, expected_meta] : s_type_test_expected_genotype_fields)
		{
			INFO("Checking for genotype field “" << id << "”");
			REQUIRE(actual_genotype_fields.end() != actual_genotype_fields.find(id));
			auto const field_it(genotype_fields_by_id.find(id));
			if (expected.unset_genotype_ids.contains(id))
				REQUIRE(genotype_fields_by_id.end() == field_it);
			else
			{
				REQUIRE(genotype_fields_by_id.end() != field_it);
				check_expected_field(first_sample, *field_it->second, expected.genotype_values, expected.empty_genotype_ids);
			}
		}
	
		// Handle GT.
		{
			auto const it(genotype_fields_by_id.find("GT"));
			if (genotype_fields_by_id.end() != it)
			{
				REQUIRE_NOTHROW([&](){
					auto const &gt_field(dynamic_cast <vcf::genotype_field_gt &>(*it->second));
					auto const &expected_gt(expected.gt);
			
					if (expected_gt.is_in_record)
					{
						REQUIRE(gt_field.has_value(first_sample));
						auto const &actual_gt(gt_field(first_sample));
						REQUIRE(actual_gt.size() == 2);
						REQUIRE(actual_gt[0].alt == expected_gt.lhs);
						REQUIRE(actual_gt[1].alt == expected_gt.rhs);
						REQUIRE(actual_gt[1].is_phased == expected_gt.is_phased);
					}
					else
					{
						REQUIRE(!gt_field.has_value(first_sample));
					}
				});
			}
		}
	}
	
	
	class test_fixture
	{
	public:
		enum class vcf_input_type : std::uint8_t
		{
			MMAP,
			STREAM,
			COMPRESSED_STREAM
		};
		
		enum class vcf_parsing_style : std::uint8_t
		{
			ALL_AT_ONCE,
			ONE_BY_ONE,
			STOP_EVERY_TIME
		};
		
		typedef vcf::seekable_stream_input <
			lb::file_istream
		>										stream_input;
		typedef vcf::stream_input <
			io::filtering_stream <io::input>
		>										filtering_stream_input;
		
	protected:
		vcf::mmap_input				m_mmap_input;
		
		stream_input				m_stream_input{128};
		
		filtering_stream_input		m_filtering_stream_input;
		lb::file_istream			m_compressed_input_stream;
		
		vcf::reader					m_reader;
		std::vector <std::string>	m_expected_info_keys{map_keys(s_type_test_expected_info_fields)};
		std::vector <std::string>	m_expected_genotype_keys{map_keys(s_type_test_expected_genotype_fields)};
	
	public:
		auto all_vcf_input_types() const
		{
			return lb::make_array <vcf_input_type>(
				vcf_input_type::MMAP,
				vcf_input_type::STREAM,
				vcf_input_type::COMPRESSED_STREAM
			);
		}
		
		auto all_vcf_parsing_styles() const
		{
			return lb::make_array <vcf_parsing_style>(
				vcf_parsing_style::ALL_AT_ONCE,
				vcf_parsing_style::ONE_BY_ONE,
				vcf_parsing_style::STOP_EVERY_TIME
			);
		}
		
		void open_vcf_file(char const *name, vcf_input_type const input_type)
		{
			switch (input_type)
			{
				case vcf_input_type::MMAP:
					m_mmap_input.handle().open(name);
					m_reader = vcf::reader(m_mmap_input);
					break;
				
				case vcf_input_type::STREAM:
					lb::open_file_for_reading(name, m_stream_input.stream());
					m_reader = vcf::reader(m_stream_input);
					break;
				
				case vcf_input_type::COMPRESSED_STREAM:
				{
					std::string compressed_file_name(name);
					compressed_file_name += ".gz";
					lb::open_file_for_reading(compressed_file_name, m_compressed_input_stream);
					auto &filtering_stream(m_filtering_stream_input.stream());
					filtering_stream.push(boost::iostreams::gzip_decompressor());
					filtering_stream.push(m_compressed_input_stream);
					filtering_stream.exceptions(std::istream::badbit);
					m_reader = vcf::reader(m_filtering_stream_input);
					break;
				}
				
				default:
					FAIL("Unexpected input type");
					break;
			}
		}
	
		void read_vcf_header()
		{
			//m_reader.fill_buffer();
			m_reader.read_header();
		}
	
		void add_reserved_info_keys()
		{
			vcf::add_reserved_info_keys(m_reader.info_fields());
			auto const reserved_info_keys(map_keys(m_reader.info_fields()));
			add_to_sorted_vector(reserved_info_keys, m_expected_info_keys);
		}
	
		void add_reserved_genotype_keys()
		{
			vcf::add_reserved_genotype_keys(m_reader.genotype_fields());
			auto const reserved_genotype_keys(map_keys(m_reader.genotype_fields()));
			add_to_sorted_vector(reserved_genotype_keys, m_expected_genotype_keys);
		}
		
		void add_reserved_keys()
		{
			add_reserved_info_keys();
			add_reserved_genotype_keys();
		}
		
		void parse(vcf_parsing_style const parsing_style, vcf::reader::callback_cq_fn const &fn)
		{
			switch (parsing_style)
			{
				case vcf_parsing_style::ALL_AT_ONCE:
				{
					m_reader.parse(fn);
					break;
				}
				
				case vcf_parsing_style::ONE_BY_ONE:
				{
					vcf::reader::parser_state state;
					bool should_continue(true);
					do
					{
						should_continue = m_reader.parse_one(fn, state);
					} while (should_continue);
					
					break;
				}
				
				case vcf_parsing_style::STOP_EVERY_TIME:
				{
					bool should_continue(false);
					auto wrapper_fn([&fn, &should_continue](vcf::transient_variant const &var){
						// If fn is actually supposed to stop, an exception could be used.
						REQUIRE(fn(var));
						should_continue = true;
						return false;
					});
					
					do
					{
						should_continue = false;
						m_reader.parse(wrapper_fn);
					} while (should_continue);
					
					break;
				}
				
				default:
					FAIL("Unexpected parsing style");
					break;
			}
		}
		
		vcf::reader &get_reader() { return m_reader; }
		vcf::reader const &get_reader() const { return m_reader; }
		std::vector <std::string> const &get_expected_info_keys() const { return m_expected_info_keys; }
		std::vector <std::string> const &get_expected_genotype_keys() const { return m_expected_genotype_keys; }
		auto const &get_actual_info_fields() const { return m_reader.info_fields(); }
		auto const &get_actual_genotype_fields() const { return m_reader.genotype_fields(); }
		std::vector <std::string> get_actual_info_keys() const { return map_keys(get_actual_info_fields()); }
		std::vector <std::string> get_actual_genotype_keys() const { return map_keys(get_actual_genotype_fields()); }
	};
}


SCENARIO("VCF reader instantiation", "[vcf_reader]")
{
	GIVEN("the reader type")
	{
		WHEN("constructed without parameters")
		{
			THEN("the reader is correctly constructed")
			{
				vcf::reader reader;
			}
		}
		
		WHEN("constructed with vcf_input")
		{
			vcf::mmap_input input;
			vcf::reader reader(input);
			
			THEN("the reader is correctly constructed")
			{
				REQUIRE(&reader.vcf_input() == &input);
			}
		}
		
		WHEN("constructed with reserved info and genotype keys")
		{
			vcf::reader reader;
			vcf::add_reserved_info_keys(reader.info_fields());
			vcf::add_reserved_genotype_keys(reader.genotype_fields());
			
			THEN("the corresponding metadata descriptions get instantiated")
			{
				auto const &info_fields(reader.info_fields());
				auto const &genotype_fields(reader.genotype_fields());
				
				metadata_map const expected_info_fields{
					{"AA",			{1,					vcf::metadata_value_type::STRING}},
					{"AC",			{vcf::VCF_NUMBER_A,	vcf::metadata_value_type::INTEGER}},
					{"AD",			{vcf::VCF_NUMBER_R,	vcf::metadata_value_type::INTEGER}},
					{"ADF",			{vcf::VCF_NUMBER_R,	vcf::metadata_value_type::INTEGER}},
					{"ADR",			{vcf::VCF_NUMBER_R,	vcf::metadata_value_type::INTEGER}},
					{"AF",			{vcf::VCF_NUMBER_A,	vcf::metadata_value_type::FLOAT}},
					{"AN",			{1,					vcf::metadata_value_type::INTEGER}},
					{"BQ",			{1,					vcf::metadata_value_type::FLOAT}},
					{"CIGAR",		{vcf::VCF_NUMBER_A,	vcf::metadata_value_type::STRING}},
					{"DB",			{0,					vcf::metadata_value_type::FLAG}},
					{"DP",			{1,					vcf::metadata_value_type::INTEGER}},
					{"END",			{1,					vcf::metadata_value_type::INTEGER}},
					{"H2",			{0,					vcf::metadata_value_type::FLAG}},
					{"H3",			{0,					vcf::metadata_value_type::FLAG}},
					{"MQ",			{1,					vcf::metadata_value_type::FLOAT}},
					{"MQ0",			{1,					vcf::metadata_value_type::INTEGER}},
					{"NS",			{1,					vcf::metadata_value_type::INTEGER}},
					{"SB",			{4,					vcf::metadata_value_type::INTEGER}},
					{"SOMATIC",		{0,					vcf::metadata_value_type::FLAG}},
					{"VALIDATED",	{0,					vcf::metadata_value_type::FLAG}},
					{"1000G",		{0,					vcf::metadata_value_type::FLAG}}
				};
				
				metadata_map const expected_genotype_fields{
					{"AD",			{vcf::VCF_NUMBER_R,	vcf::metadata_value_type::INTEGER}},
					{"ADF",			{vcf::VCF_NUMBER_R,	vcf::metadata_value_type::INTEGER}},
					{"ADR",			{vcf::VCF_NUMBER_R,	vcf::metadata_value_type::INTEGER}},
					{"DP",			{1,					vcf::metadata_value_type::INTEGER}},
					{"EC",			{vcf::VCF_NUMBER_A,	vcf::metadata_value_type::INTEGER}},
					{"FT",			{1,					vcf::metadata_value_type::STRING}},
					{"GL",			{vcf::VCF_NUMBER_G,	vcf::metadata_value_type::FLOAT}},
					{"GP",			{vcf::VCF_NUMBER_G,	vcf::metadata_value_type::FLOAT}},
					{"GQ",			{1,					vcf::metadata_value_type::INTEGER}},
					{"GT",			{1,					vcf::metadata_value_type::STRING}},
					{"HQ",			{2,					vcf::metadata_value_type::INTEGER}},
					{"MQ",			{1,					vcf::metadata_value_type::INTEGER}},
					{"PL",			{vcf::VCF_NUMBER_G,	vcf::metadata_value_type::INTEGER}},
					{"PQ",			{1,					vcf::metadata_value_type::INTEGER}},
					{"PS",			{1,					vcf::metadata_value_type::INTEGER}}
				};
				
				check_metadata_fields(expected_info_fields, info_fields, false);
				check_metadata_fields(expected_genotype_fields, genotype_fields, false);
			}
		}
	}
}


SCENARIO("The VCF reader can report EOF correctly", "[vcf_reader]")
{
	test_fixture fixture;
	
	auto const vcf_input_type = GENERATE(
		test_fixture::vcf_input_type::MMAP,
		test_fixture::vcf_input_type::STREAM,
		test_fixture::vcf_input_type::COMPRESSED_STREAM
	);
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-simple.vcf", vcf_input_type);
		
		WHEN("the file is parsed")
		{
			fixture.read_vcf_header();
			auto &reader(fixture.get_reader());
			reader.set_parsed_fields(vcf::field::ALL);
			
			THEN("parse_one returns the correct status")
			{
				vcf::reader::parser_state state;
				REQUIRE(true == reader.parse_one([](vcf::transient_variant const &var){ return true; }, state));
				REQUIRE(false == reader.parse_one([](vcf::transient_variant const &var){ return true; }, state));
			}
		}
	}
}


SCENARIO("The VCF reader can parse VCF header", "[vcf_reader]")
{
	test_fixture fixture;
	
	auto const vcf_input_type = GENERATE(
		test_fixture::vcf_input_type::MMAP,
		test_fixture::vcf_input_type::STREAM,
		test_fixture::vcf_input_type::COMPRESSED_STREAM
	);
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-data-types.vcf", vcf_input_type);
		// Don’t add the reserved fields.
		
		WHEN("the headers of the file are parsed")
		{
			fixture.read_vcf_header();
			
			THEN("the corresponding metadata descriptions get instantiated")
			{
				auto const &info_fields(fixture.get_actual_info_fields());
				auto const &genotype_fields(fixture.get_actual_genotype_fields());
				
				auto expected_genotype_fields(s_type_test_expected_genotype_fields);
				expected_genotype_fields.emplace("GT", metadata_description(1, vcf::metadata_value_type::STRING));
				
				check_metadata_fields(s_type_test_expected_info_fields, info_fields, true);
				check_metadata_fields(expected_genotype_fields, genotype_fields, true);
			}
		}
	}
}


// FIXME: combine the following two.
SCENARIO("The VCF reader can parse simple VCF records", "[vcf_reader]")
{
	test_fixture fixture;
	auto const vcf_input_types(fixture.all_vcf_input_types());
	auto const vcf_parsing_styles(fixture.all_vcf_parsing_styles());
	auto const [
		vcf_input_type,
		vcf_parsing_style
	] = GENERATE_REF(from_range(rsv::cartesian_product(vcf_input_types, vcf_parsing_styles)));
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-simple.vcf", vcf_input_type);
		
		WHEN("the file is parsed")
		{
			fixture.read_vcf_header();
			fixture.get_reader().set_parsed_fields(vcf::field::ALL);
			
			THEN("each record matches the expected")
			{
				auto const expected_records(prepare_expected_records_for_test_simple_vcf());
				
				std::size_t idx{};
				fixture.parse(
					vcf_parsing_style,
					[
						&expected_records,
						&fixture,
						&idx
					]
					(vcf::transient_variant const &var){
						
						REQUIRE(idx < expected_records.size());
						auto const &expected(expected_records[idx]);
						
						check_record_against_expected_in_test_simple_vcf(var, expected);
						
						++idx;
						return true;
					}
				);
				
				REQUIRE(idx == expected_records.size());
			}
		}
	}
}

SCENARIO("The VCF reader can parse VCF records", "[vcf_reader]")
{
	test_fixture fixture;
	
	auto const vcf_input_types(fixture.all_vcf_input_types());
	auto const vcf_parsing_styles(fixture.all_vcf_parsing_styles());
	auto const [
		vcf_input_type,
		vcf_parsing_style
	] = GENERATE_REF(from_range(rsv::cartesian_product(vcf_input_types, vcf_parsing_styles)));
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-data-types.vcf", vcf_input_type);
		
		// Add to get GT.
		fixture.add_reserved_keys();
		
		WHEN("the file is parsed")
		{
			fixture.read_vcf_header();
			fixture.get_reader().set_parsed_fields(vcf::field::ALL);
			
			THEN("each record matches the expected")
			{
				auto const expected_info_keys(fixture.get_expected_info_keys());
				auto const expected_genotype_keys(fixture.get_expected_genotype_keys());
				auto const actual_info_keys(fixture.get_actual_info_keys());
				auto const actual_genotype_keys(fixture.get_actual_genotype_keys());
				
				REQUIRE(actual_info_keys == expected_info_keys);
				REQUIRE(actual_genotype_keys == expected_genotype_keys);
				
				auto const expected_records(prepare_expected_records_for_test_data_types_vcf());
				
				std::size_t idx{};
				fixture.parse(
					vcf_parsing_style,
					[
						&expected_records,
						&fixture,
						&idx
					]
					(vcf::transient_variant const &var){
						
						REQUIRE(idx < expected_records.size());
						auto const &expected(expected_records[idx]);
						
						check_record_against_expected_in_test_data_types_vcf(
							var,
							expected,
							fixture.get_actual_info_fields(),
							fixture.get_actual_genotype_fields()
						);
						
						++idx;
						return true;
					}
				);
				
				REQUIRE(idx == expected_records.size());
			}
		}
	}
}


SCENARIO("Transient VCF records can be copied to persistent ones", "[vcf_reader]")
{
	test_fixture fixture;
	
	auto const vcf_input_types(fixture.all_vcf_input_types());
	auto const vcf_parsing_styles(fixture.all_vcf_parsing_styles());
	auto const bool_values(lb::make_array <bool>(true, false));
	auto const bool_values_(lb::make_array <bool>(true));
	auto const [
		vcf_input_type,
		should_add_reserved_keys,
		should_use_copy_constructor_for_copying_variant,
		vcf_parsing_style
	] = GENERATE_REF(from_range(rsv::cartesian_product(vcf_input_types, bool_values, bool_values_, vcf_parsing_styles))); // FIXME: Using the copy assignment operator does not work b.c. the target variant’s reserve_memory_for_samples() is not called.
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-data-types.vcf", vcf_input_type);
		if (should_add_reserved_keys)
			fixture.add_reserved_keys();
		
		WHEN("the file is parsed")
		{
			fixture.read_vcf_header();
			fixture.get_reader().set_parsed_fields(vcf::field::ALL);
			
			THEN("the variant records may be copied")
			{
				fixture.parse(
					vcf_parsing_style,
					[
						should_use_copy_constructor_for_copying_variant = should_use_copy_constructor_for_copying_variant,
						&fixture
					](vcf::transient_variant const &var){
						
						if (should_use_copy_constructor_for_copying_variant)
							vcf::variant persistent_variant(var);
						else
						{
							auto persistent_variant(fixture.get_reader().make_empty_variant());
							persistent_variant = var;
						}
						return true;
					}
				);
			}
		}
	}
}


SCENARIO("Persistent VCF records can be used to access the variant data", "[vcf_reader]")
{
	test_fixture fixture;
	
	auto const vcf_input_types(fixture.all_vcf_input_types());
	auto const vcf_parsing_styles(fixture.all_vcf_parsing_styles());
	auto const bool_values(lb::make_array <bool>(true, false));
	auto const bool_values_(lb::make_array <bool>(true));
	auto const [
		vcf_input_type,
		should_add_reserved_keys,
		should_use_copy_constructor_for_copying_variant,
		vcf_parsing_style
	] = GENERATE_REF(from_range(rsv::cartesian_product(vcf_input_types, bool_values, bool_values_, vcf_parsing_styles))); // FIXME: Using the copy assignment operator does not work b.c. the target variant’s reserve_memory_for_samples() is not called.
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-data-types.vcf", vcf_input_type);
		if (should_add_reserved_keys)
			fixture.add_reserved_keys();
		
		WHEN("the variant records have been copied")
		{
			fixture.read_vcf_header();
			fixture.get_reader().set_parsed_fields(vcf::field::ALL);
			
			auto const &actual_info_fields(fixture.get_actual_info_fields());
			auto const &actual_genotype_fields(fixture.get_actual_genotype_fields());
			
			std::vector <vcf::variant> persistent_variants;
			fixture.parse(
				vcf_parsing_style,
				[
					should_use_copy_constructor_for_copying_variant = should_use_copy_constructor_for_copying_variant,
					&fixture,
					&persistent_variants
				](vcf::transient_variant const &var){
					if (should_use_copy_constructor_for_copying_variant)
						persistent_variants.emplace_back(var);
					else
					{
						auto persistent_variant(fixture.get_reader().make_empty_variant());
						persistent_variant = var;
						persistent_variants.emplace_back(std::move(persistent_variant));
					}
					return true;
				}
			);
			
			THEN("each copied record matches the expected")
			{
				auto const expected_records(prepare_expected_records_for_test_data_types_vcf());
				for (auto const &[idx, var] : rsv::enumerate(persistent_variants))
				{
					REQUIRE(idx < expected_records.size());
					auto const &expected(expected_records[idx]);
					
					check_record_against_expected_in_test_data_types_vcf(
						var,
						expected,
						fixture.get_actual_info_fields(),
						fixture.get_actual_genotype_fields()
					);
				}
			}
		}
	}
}


SCENARIO("Copying persistent variants works even if the format has changed", "[vcf_reader]")
{
	test_fixture fixture;
	
	auto const vcf_input_types(fixture.all_vcf_input_types());
	auto const vcf_parsing_styles(fixture.all_vcf_parsing_styles());
	auto const bool_values(lb::make_array <bool>(true, false));
	auto const bool_values_(lb::make_array <bool>(true));
	auto const [
		vcf_input_type,
		should_add_reserved_keys,
		should_use_copy_constructor_for_copying_variant,
		vcf_parsing_style
	] = GENERATE_REF(from_range(rsv::cartesian_product(vcf_input_types, bool_values, bool_values_, vcf_parsing_styles))); // FIXME: Using the copy assignment operator does not work b.c. the target variant’s reserve_memory_for_samples() is not called.
	
	GIVEN("a VCF file")
	{
		fixture.open_vcf_file("test-files/test-data-types.vcf", vcf_input_type);
		if (should_add_reserved_keys)
			fixture.add_reserved_keys();
		
		WHEN("the file is parsed")
		{
			fixture.read_vcf_header();
			fixture.get_reader().set_parsed_fields(vcf::field::ALL);
			
			THEN("the variant records may be copied")
			{
				auto const &actual_info_fields(fixture.get_actual_info_fields());
				auto const &actual_genotype_fields(fixture.get_actual_genotype_fields());
				auto const expected_records(prepare_expected_records_for_test_data_types_vcf());
				
				std::size_t idx{};
				
				vcf::variant dst_variant;
				if (!should_use_copy_constructor_for_copying_variant)
					dst_variant = fixture.get_reader().make_empty_variant();
				
				fixture.parse(
					vcf_parsing_style,
					[
						should_use_copy_constructor_for_copying_variant = should_use_copy_constructor_for_copying_variant,
						&actual_info_fields,
						&actual_genotype_fields,
						&expected_records,
						&idx,
						&dst_variant
					](vcf::transient_variant const &var){
				
						REQUIRE(idx < expected_records.size());
						auto const &expected(expected_records[idx]);
				
						if (should_use_copy_constructor_for_copying_variant)
						{
							vcf::variant persistent_variant(var);
							dst_variant = persistent_variant;
				
							check_record_against_expected_in_test_data_types_vcf(persistent_variant, expected, actual_info_fields, actual_genotype_fields);
						}
						else
						{
							dst_variant = var;
						}
				
						check_record_against_expected_in_test_data_types_vcf(var, expected, actual_info_fields, actual_genotype_fields);
						check_record_against_expected_in_test_data_types_vcf(dst_variant, expected, actual_info_fields, actual_genotype_fields);
				
						++idx;
						return true;
					}
				);
			}
		}
	}
}

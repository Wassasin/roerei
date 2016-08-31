#pragma once

#include <fstream>

#include <roerei/storage.hpp>

#include <sys/stat.h>
#include <stdlib.h>
#include <boost/filesystem.hpp>

namespace roerei
{
	class diff
	{
	private:
		static void write_names(dataset_t const& d, std::string const& path)
		{
			std::ofstream os(path);

			d.objects.iterate([&os](object_id_t, uri_t const& obj_uri) {
				os << "O " << obj_uri << std::endl;
			});
			d.features.iterate([&os](feature_id_t, uri_t const& f_uri) {
				os << "F " << f_uri << std::endl;
			});
			d.dependencies.iterate([&os](dependency_id_t, uri_t const& d_uri) {
				os << "D " << d_uri << std::endl;
			});
		}

		static void write_complete(dataset_t const& d, std::string const& path)
		{
			std::ofstream os(path);

			d.objects.iterate([&os, &d](object_id_t obj_id, uri_t const& obj_uri) {
				os << obj_uri << std::endl;

				for(auto fp : d.feature_matrix[obj_id])
					os << "  F " << ' ' << d.features[fp.first] << std::endl;
				
				for(auto dp : d.dependency_matrix[obj_id])
					os << "  D " << ' ' << d.dependencies[dp.first] << std::endl;
			});
		}

	public:
		/* Write orig and new to files in /tmp, and run $ git diff --no-index <orig> <new>.
		 * Clean up afterwards.
		 */
		static void exec(std::string const& corpus_orig, std::string const& corpus_new)
		{
			auto c_orig = storage::read_dataset(corpus_orig);
			auto c_new = storage::read_dataset(corpus_new);
			
			std::string prefix = "/tmp/diff-";
			prefix += boost::filesystem::unique_path().native();

			boost::filesystem::create_directory(prefix.c_str());

			{
				std::string path_complete_orig(prefix + "/complete-" + corpus_orig);
				std::string path_complete_new(prefix + "/complete-" + corpus_new);

				write_complete(c_orig, path_complete_orig);
				write_complete(c_new, path_complete_new);
				
				std::string path_names_orig(prefix + "/names-" + corpus_orig);
				std::string path_names_new(prefix + "/names-" + corpus_new);

				write_names(c_orig, path_names_orig);
				write_names(c_new, path_names_new);
				
				{
					std::string cmd("git diff --no-index ");
					cmd += path_complete_orig + " " + path_complete_new;
					system(cmd.c_str());
				}

				{
					std::string cmd("git diff --no-index ");
					cmd += path_names_orig + " " + path_names_new;
					system(cmd.c_str());
				}

				{
					std::string cmd("git diff --no-index --stat ");
					cmd += path_complete_orig + " " + path_complete_new;
					system(cmd.c_str());
				}
				
				{
					std::string cmd("git diff --no-index --stat ");
					cmd += path_names_orig + " " + path_names_new;
					system(cmd.c_str());
				}
			}

			boost::filesystem::remove_all(prefix);
		}
	};
}

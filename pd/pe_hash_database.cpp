#include "StdAfx.h"
#include "pe_hash_database.h"

bool pe_hash_database::_is_mz(FILE* stream)
{
	char twochars[2];
	if( (fread( &twochars, 1, 2, stream) == 2) && twochars[0] == 'M' && twochars[1] == 'Z' )
	{
		fseek( stream, 0, SEEK_SET);
		return true;
	}
	fseek( stream, 0, SEEK_SET);
	return false;
}

pe_hash_database::pe_hash_database(char* clean_database_name)
{
	// Build the full database name
	string path = ExePath();
	//const char* path = ExePath().c_str();
	_clean_database_path = new char[ path.length() + strlen(clean_database_name) + 2 ];

	sprintf( _clean_database_path, "%s\\%s", path.c_str() , clean_database_name );

	_clean_hashes.rehash(0x10000);

	// Open and read in the database of clean hashes if it exists.
	FILE* fh = fopen( _clean_database_path, "rb" );
	unsigned __int64 hash;
	if( fh )
	{
		// Read in the database
		while( !feof( fh ) )
		{
			 if( fread( &hash, sizeof(unsigned __int64), 1, fh ) == 1 )
			 {
				_clean_hashes.insert( hash );
			 }
		}
		fclose(fh);

		printf("Loaded %i clean hashes from database.\n", _clean_hashes.size());
	}
	else
	{
		if( PathFileExistsA( _clean_database_path ) )
		{
			// Do not continue if the database exists, but we failed to open it. This is to protect
			// the contents of the database from being overwritten if we save the now empty database
			// successfully.
			PrintLastError(L"Failed to open existing hash database. Terminating.");
			exit(-1);
		}
	}
}

int pe_hash_database::count()
{
	return _clean_hashes.size();
}

bool pe_hash_database::clear_database()
{
	_clean_hashes.clear();
	return true;
}

bool pe_hash_database::add_hashes(DynArray<unsigned __int64> hashes)
{
	if( hashes.GetSize() > 0 )
	{
		// Add all the headers as hash exclusions
		for( int i = 0; i < hashes.GetSize(); i++ )
		{
			if( hashes[i] != 0 )
			{
				if( _clean_hashes.count( hashes[i] ) == 0 )
				{
					_clean_hashes.insert( hashes[i] );
				}
			}
		}
	}

	return true;
}

	

bool pe_hash_database::add_folder( char* dir_name, WCHAR* filter, bool recursively )
{
	// Expand the environment names in the directory
	char* dir_name_expanded = new char[1000];
	ExpandEnvironmentStringsA( dir_name, dir_name_expanded, 1000 );

	DIR *dir;
	struct dirent *ent;
	dir = opendir (dir_name_expanded);
	if (dir != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			// Convert the path to wchar format
			wchar_t* result = new wchar_t[ent->d_namlen + 1];

			if( result != NULL )
			{
				for( int i = 0; i < ent->d_namlen; i++ )
					result[i] = ent->d_name[i];
				result[ent->d_namlen] = 0;

				if( (ent->d_type & DT_DIR) )
				{
					// Process this subdirectory if recursive flag is on
					if( recursively && wcscmp(result, L".") != 0 && wcscmp(result, L"..") != 0  )
					{
						// Build the directory path
						char* directory = new char[strlen(dir_name_expanded) + strlen(ent->d_name) + 2];
						sprintf(directory, "%s/%s", dir_name_expanded, ent->d_name);

						add_folder( directory, filter, recursively );

						// Cleanup
						delete[] directory;
					}
				}else{
					// Check if this filename is a match to the specified pattern
					if( PathMatchSpec( result, filter ) )
					{
						// Process this file
						int length = wcslen(result) + strlen(dir_name_expanded) + 1;
						char* filename = new char[length + 1];
						filename[length] = 0;
						sprintf( filename, "%s\\%S", dir_name_expanded, result );

						// Processes the specified file
						FILE* fh = fopen( filename, "rb" );
						if( fh != NULL )
						{
							if( _is_mz( fh ) )
							{
								fclose(fh);

								add_file(filename);
							}
							else
								fclose(fh);
						}else{
							// Error
							fprintf(stderr, "Error opening file %s: %s.\n", filename, strerror(errno));
						}
						delete[] filename;
					}
				}
				delete[] result;
			}
			else
			{
				fprintf(stderr, "Failed to allocate memory block of size %i for filename: %s.\n", ent->d_namlen + 1, strerror(errno));
			}
		}
		closedir (dir);
		return true;
	}else{
		fprintf(stderr, "Unable to open directory %s: %s.\n", dir_name_expanded, strerror(errno));
	}
	return false;
}


bool pe_hash_database::remove_folder( char* dir_name, WCHAR* filter, bool recursively )
{
	// Expand the environment names in the directory
	char* dir_name_expanded = new char[1000];
	ExpandEnvironmentStringsA( dir_name, dir_name_expanded, 1000 );


	DIR *dir;
	struct dirent *ent;
	dir = opendir (dir_name_expanded);
	if (dir != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) {
			// Convert the path to wchar format
			wchar_t* result = new wchar_t[ent->d_namlen + 1];

			if( result != NULL )
			{
				for( int i = 0; i < ent->d_namlen; i++ )
					result[i] = ent->d_name[i];
				result[ent->d_namlen] = 0;

				if( (ent->d_type & DT_DIR) )
				{
					// Process this subdirectory if recursive flag is on
					if( recursively && wcscmp(result, L".") != 0 && wcscmp(result, L"..") != 0  )
					{
						// Build the directory path
						char* directory = new char[strlen(dir_name_expanded) + strlen(ent->d_name) + 2];
						sprintf(directory, "%s/%s", dir_name_expanded, ent->d_name);

						remove_folder( directory, filter, recursively );

						// Cleanup
						delete[] directory;
					}
				}else{
					// Check if this filename is a match to the specified pattern
					if( PathMatchSpec( result, filter ) )
					{
						// Process this file
						int length = wcslen(result) + strlen(dir_name_expanded) + 1;
						char* filename = new char[length + 1];
						filename[length] = 0;
						sprintf( filename, "%s\\%S", dir_name_expanded, result );

						// Processes the specified file
						FILE* fh = fopen( filename, "rb" );
						if( fh != NULL )
						{
							if( _is_mz( fh ) )
							{
								fclose(fh);

								remove_file(filename);
							}
							else
								fclose(fh);
						}else{
							// Error
							fprintf(stderr, "Error opening file %s: %s.\n", filename, strerror(errno));
						}
						delete[] filename;
					}
				}
				delete[] result;
			}
			else
			{
				fprintf(stderr, "Failed to allocate memory block of size %i for filename: %s.\n", ent->d_namlen + 1, strerror(errno));
			}
		}
		closedir (dir);
		return true;
	}else{
		fprintf(stderr, "Unable to open directory %s: %s.\n", dir_name_expanded, strerror(errno));
	}
	return false;
}


bool pe_hash_database::contains(unsigned __int64 hash)
{
	return _clean_hashes.count( hash ) != 0;
}

bool pe_hash_database::add_file(char* file)
{
	PD_OPTIONS options;
	options.ForceGenHeader = false;
	options.ImportRec = false;
	options.Verbose = false;
	pe_header* header = new pe_header(file, options);
	unsigned __int64 hash = 0;
	header->process_pe_header();
	header->process_sections();
	
	if( header->somewhat_parsed() )
	{
		hash = header->get_hash();
	}
	else
	{
		printf("Failed to parse PE header for %s.\n", file);
		delete header;
		return false;
	}
	
	
	delete header;

	if( hash != 0 )
	{
		if( _clean_hashes.count( hash ) == 0 )
		{
			_clean_hashes.insert( hash );
			printf("...new hash %s,0x%llX\n", file, hash);
		}
		
		return true;
	}

	printf("Failed to calculate hash for file %s.\n", file);
	return false;
}

bool pe_hash_database::remove_file(char* file)
{
	PD_OPTIONS options;
	options.ForceGenHeader = false;
	options.ImportRec = false;
	options.Verbose = false;
	pe_header* header = new pe_header(file, options);
	header->process_pe_header();
	header->process_sections();
	
	unsigned __int64 hash = 0;
	if( header->somewhat_parsed() )
	{
		hash = header->get_hash();
	}
	delete header;

	if( hash != 0 )
	{
		if( _clean_hashes.count( hash ) != 0 )
		{
			_clean_hashes.erase( hash );
			printf("...deleted hash %s,0x%llX\n", file, hash);
		}
		
		return true;
	}

	printf("Failed to add hash for file %s.\n", file);
	return false;
}

bool pe_hash_database::save()
{
	// Open and read in the database of clean hashes if it exists.
	FILE* fh = fopen( _clean_database_path, "wb" );
	unsigned __int64 hash;
	if( fh )
	{
		// Write the database
		for (unordered_set<unsigned __int64>::const_iterator it = _clean_hashes.begin();
			it != _clean_hashes.end(); ++it) 
		{
			hash = *it;
			fwrite( &hash, sizeof(unsigned __int64), 1, fh );
		}
		fclose(fh);

		printf("Wrote to clean hash database. It now has a total of %i clean hashes.\n", _clean_hashes.size());

		return true;
	}
	else
	{
		PrintLastError(L"Failed to open existing hash database..");
	}

	return false;
}

pe_hash_database::~pe_hash_database(void)
{
	delete[] _clean_database_path;
}


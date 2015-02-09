#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include "include/zlib.h"
#include <dirent.h>
#include <iomanip>

//add ability to take in output directory

using namespace std;

bool GetDataStream( string, std::stringstream&);
bool RecursiveConversion( string inDir, string outDir);
bool DirectoryExists( const char* pzPath );
void SetDataStream( string, std::stringstream&, bool ascii);

int main(int argc, char **argv)
{
    string fileName, outDirName;
    std::stringstream stream;

    if(argc==2)
    {
        fileName=argv[1];
        outDirName=fileName.substr(0,fileName.find_last_of('/')+1);
    }
    else if(argc==3)
    {
        fileName=argv[1];
        outDirName=argv[2];

        if(!(DirectoryExists((outDirName).c_str())))
        {
            system( ("mkdir -p -m=666 "+outDirName).c_str());
            if(DirectoryExists((outDirName).c_str()))
            {
                cout << "created directory " << outDirName << "\n" << endl;
            }
            else
            {
                cout << "\nError: could not create Directory " << outDirName << "\n" << endl;
                return 1;
            }
        }
    }

    if(fileName.back()=='/')
    {
        RecursiveConversion( fileName, outDirName);
    }
    else
    {
        // Gets data from the file and stores it into a data stream
        if(GetDataStream(fileName, stream))
        {
            return 1;
        }
        outDirName=outDirName+fileName.substr(fileName.find_last_of('/')+1, string::npos);
        SetDataStream( outDirName, stream, true);
    }

    return 0;

}

bool RecursiveConversion( string inDir, string outDir)
{
    DIR *dir;
    struct dirent *ent;
    stringstream stream;
    bool test;
    string inFile, outFile;

    if(inDir.back()!='/')
    {
        inDir.push_back('/');
    }
    if(outDir.back()!='/')
    {
        outDir.push_back('/');
    }

    //goes through the given directory and converts the ENDF libraries that match the given vversion
    if ((dir = opendir (inDir.c_str())) != NULL)
    {
        while ((ent = readdir (dir)) != NULL)
        {
            if((string(ent->d_name)!="..")&&(string(ent->d_name)!="."))
            {
                inFile=inDir+ent->d_name;
                outFile=outDir+ent->d_name;

                test=RecursiveConversion( inFile, outFile);
            }

        }
        closedir(dir);
    }
    else
    {
        inDir.pop_back();
        outDir.pop_back();
        inFile=inDir;
        outFile=outDir;
        inDir=inDir.substr(0,inDir.find_last_of('/')+1);
        outDir=outDir.substr(0,outDir.find_last_of('/')+1);

        // Gets data from the file and stores it into a data stream
        GetDataStream(inFile, stream);
        if(stream.good())
        {
            if(!(DirectoryExists((outDir).c_str())))
            {
                system( ("mkdir -p -m=666 "+outDir).c_str());
                if(DirectoryExists((outDir).c_str()))
                {

                }
                else
                {
                    cout << "\nError: could not create Directory " << outDir << "\n" << endl;
                    return 1;
                }
            }
            SetDataStream( outFile, stream, true);
            stream.str("");
            stream.clear();
            test=true;
        }
        else
        {
            test=false;
        }

    }
    return test;
}

bool DirectoryExists( const char* pzPath )
{
    if ( pzPath == NULL) return false;

    DIR *pDir;
    bool bExists = false;

    pDir = opendir (pzPath);

    if (pDir != NULL)
    {
        bExists = true;
        closedir (pDir);
    }

    return bExists;
}

bool GetDataStream( string filename , std::stringstream& ss)
{
   string* data=NULL;
   std::ifstream* in=NULL;
   //string compfilename(filename);

   if(filename.substr((filename.length()-2),2)==".z")
   {
        in = new std::ifstream ( filename.c_str() , std::ios::binary | std::ios::ate );
   }

   if ( in!=NULL && in->good() )
   {
// Use the compressed file
      uLongf file_size = (uLongf)(in->tellg());
      in->seekg( 0 , std::ios::beg );
      Bytef* compdata = new Bytef[ file_size ];

      while ( *in )
      {
         in->read( (char*)compdata , file_size );
      }

      uLongf complen = (uLongf) ( file_size*4 );
      Bytef* uncompdata = new Bytef[complen];

      while ( Z_OK != uncompress ( uncompdata , &complen , compdata , file_size ) )
      {
         delete[] uncompdata;
         complen *= 2;
         uncompdata = new Bytef[complen];
      }
      delete [] compdata;
      //                                 Now "complen" has uncomplessed size
      data = new string ( (char*)uncompdata , (long)complen );
      delete [] uncompdata;
   }
   else {
// Use regular text file
      std::ifstream thefData( filename.c_str() , std::ios::in | std::ios::ate );
      if ( thefData.good() )
      {
         int file_size = thefData.tellg();
         thefData.seekg( 0 , std::ios::beg );
         char* filedata;
         if(file_size>0)
            filedata = new char[ file_size ];
         else
         {
            if(in!=NULL)
            {
                in->close();
                delete in;
            }
            return false;
         }
         while ( thefData )
         {
            thefData.read( filedata , file_size );
         }
         thefData.close();
         data = new string ( filedata , file_size );
         delete [] filedata;
      }
      else
      {
// found no data file
//                 set error bit to the stream
         ss.setstate( std::ios::badbit );
         cout << endl << "### failed to open ascii file " << filename << " ###" << endl;
      }
   }
   if (data != NULL)
   {
        ss.str(*data);
        if(data->back()!='\n')
            ss << "\n";
        ss.seekg( 0 , std::ios::beg );
    }

   if(in!=NULL)
   {
        in->close();
        delete in;
   }
   delete data;

   if(ss.good())
   {
      return true;
   }
   else
   {
      return false;
   }
}


void SetDataStream( string filename , std::stringstream& ss, bool ascii )
{
    //bool cond=true;
   if (!ascii)
   {
        string compfilename(filename);

        if(compfilename.back()!='z')
            compfilename += ".z";

       std::ofstream* out = new std::ofstream ( compfilename.c_str() , std::ios::binary | std::ios::trunc);
       if ( ss.good() )
       {
       //
    // Create the compressed file
          ss.seekg( 0 , std::ios::end );
          uLongf file_size = (uLongf)(ss.tellg());
          ss.seekg( 0 , std::ios::beg );
          Bytef* uncompdata = new Bytef[ file_size ];

          while ( ss ) {
              ss.read( (char*)uncompdata , file_size );
          }

          uLongf complen = compressBound(file_size);

          Bytef* compdata = new Bytef[complen];

          if ( Z_OK == compress ( compdata , &complen , uncompdata , file_size ) )
          {
            out->write((char*)compdata, (long)complen);
            if (out->fail())
            {
                cout << endl << "writing the compressed data to the output file " << compfilename << " failed" << endl
                    << " may not have permission to delete an older version of the file" << endl;
            }
          }
          else
          {
            cout << endl << "compressing the data failed" << endl;
          }

          delete [] uncompdata;
          delete [] compdata;
       }
       else
       {
            cout << endl << "### failed to write to binary file ###" << endl;
       }

       out->close(); delete out;
   }
   else
   {
// Use regular text file
    string compfilename(filename);

    if(compfilename.substr((compfilename.length()-2),2)==".z")
    {
        compfilename.pop_back();
        compfilename.pop_back();
    }

      std::ofstream out( compfilename.c_str() , std::ios::out | std::ios::trunc );
      if ( ss.good() )
      {
         ss.seekg( 0 , std::ios::end );
         int file_size = ss.tellg();
         ss.seekg( 0 , std::ios::beg );
         char* filedata = new char[ file_size ];
         while ( ss ) {
            ss.read( filedata , file_size );
            if(!file_size)
            {
                cout << "\n #### Error the size of the stringstream is invalid ###" << endl;
                break;
            }
         }
         out.write(filedata, file_size);
         if (out.fail())
        {
            cout << endl << "writing the ascii data to the output file " << compfilename << " failed" << endl
                 << " may not have permission to delete an older version of the file" << endl;
        }
         out.close();
         delete [] filedata;
      }
      else
      {
// found no data file
//                 set error bit to the stream
         ss.setstate( std::ios::badbit );

         cout << endl << "### failed to write to ascii file " << compfilename << " ###" << endl;
      }
   }
   ss.str("");
}

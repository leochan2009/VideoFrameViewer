/*  YUView - YUV player with advanced analytics toolset
*   Copyright (C) 2015  Institut für Nachrichtentechnik
*                       RWTH Aachen University, GERMANY
*
*   YUView is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   YUView is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with YUView.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef YUVFILE_H
#define YUVFILE_H

#include <QString>
#include <QFile>
#include <QFileInfo>
#include "yuvsource.h"

class YUVFile : public YUVSource
{
public:
    explicit YUVFile(const QString &fname);

    ~YUVFile();

    void getFormat(int* width, int* height, int* numFrames, double* frameRate);
	bool isFormatValid();

  // reads one frame in YUV444 into target byte array
  virtual void getOneFrame( QByteArray* targetByteArray, unsigned int frameIdx );
  
  void formatFromFilename(QString name, int &width, int &height, int &frameRate, int &bitDepth, int &subFormat);

	// Return the source file name
	virtual QString getName();

    //  methods for querying file information
    virtual QString getPath() { QFileInfo fileInfo(*p_srcFile); return fileInfo.filePath(); }
    virtual QString getCreatedtime() { return p_createdtime; }
    virtual QString getModifiedtime() { return p_modifiedtime; }
	virtual qint64  getNumberBytes() { return p_fileSize; }
    virtual QString getStatus();

	// Get the number of frames from the file size
	qint64 getNumberFrames();

	void setSize(int width, int height);

private:

    QFile *p_srcFile;

	// Info on the source file. Will be set when creating this object.
    QString p_path;
    QString p_createdtime;
    QString p_modifiedtime;
	qint64  p_fileSize;

    qint64 readFrame( QByteArray *targetBuffer, unsigned int frameIdx, int width, int height );
	    
	// Try to get the format the file name or the frame correlation of the first two frames
	void formatFromFile();
	void formatFromCorrelation();

    void readBytes( char* targetBuffer, qint64 startPos, qint64 length );

};

#endif // YUVFILE_H

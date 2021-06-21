#ifndef GZIPDATASOURCE_H
#define GZIPDATASOURCE_H

#include <zlib.h>
#include "uvutil.h"


enum GDState { Streaming, Finishing, Done };

class GZipDataSource : public DataSource {
  std::shared_ptr<DataSource> _pData;
  z_stream _zstrm;
  uv_buf_t _inputBuf;
  GDState _state;

public:
  GZipDataSource(std::shared_ptr<DataSource> pData);

  ~GZipDataSource();

  uint64_t size() const;
  uv_buf_t getData(size_t bytesDesired);
  void freeData(uv_buf_t buffer);
  void close();

private:
  void deflateNext();
  bool freeInputBuffer(bool force = false);
};

#endif // GZIPDATASOURCE_H

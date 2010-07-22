<?php

include_once 'base.php';

pre("#include <zip.h>");

///////////////////////////////////////////////////////////////////////////////

c('ZipArchive', null, array('Sweepable' => 'internal'),
  array(
    m(PublicMethod, "__construct"),
    m(PublicMethod, 'addEmptyDir', Boolean,
      array('dirname' => String)),
    m(PublicMethod, 'addFile', Boolean,
      array('filename' => String,
            'localname' => array(String, 'null_string'))),
    m(PublicMethod, 'addFromString', Boolean,
      array('localname' => String,
            'contents' => String)),
    m(PublicMethod, 'close', Boolean,
      array()),
    m(PublicMethod, 'deleteIndex', Boolean,
      array('index' => Int64)),
    m(PublicMethod, 'deleteName', Boolean,
      array('name' => String)),
    m(PublicMethod, 'extractTo', Boolean,
      array('destination' => String,
            'entries' => array(Variant, 'null_variant'))),
    m(PublicMethod, 'getArchiveComment', Variant,
      array()),
    m(PublicMethod, 'getCommentIndex', Variant,
      array('index' => Int64,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'getCommentName', Variant,
      array('name' => String,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'getFromIndex', Variant,
      array('index' => Int64,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'getFromName', Variant,
      array('name' => String,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'getNameIndex', Variant,
      array('index' => Int64)),
    m(PublicMethod, 'getStatusString', Variant,
      array()),
    m(PublicMethod, 'getStream', Variant,
      array('name' => String)),
    m(PublicMethod, 'locateName', Variant,
      array('name' => String,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'open', Variant,
      array('filename' => String,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'renameIndex', Boolean,
      array('index' => Int64,
            'newname' => String)),
    m(PublicMethod, 'renameName', Boolean,
      array('name' => String,
            'newname' => String)),
    m(PublicMethod, 'setArchiveComment', Variant,
      array('comment' => String)),
    m(PublicMethod, 'setCommentIndex', Variant,
      array('index' => Int64,
            'comment' => String)),
    m(PublicMethod, 'setCommentName', Variant,
      array('name' => String,
            'comment' => String)),
    m(PublicMethod, 'statIndex', Variant,
      array('index' => Int64,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'statName', Variant,
      array('name' => String,
            'flags' => array(Int64, '0'))),
    m(PublicMethod, 'unchangeAll', Variant,
      array()),
    m(PublicMethod, 'unchangeArchive', Variant,
      array()),
    m(PublicMethod, 'unchangeIndex', Variant,
      array('index' => Int64)),
    m(PublicMethod, 'unchangeName', Variant,
      array('name' => String))
  ),
  array(
   ck("CREATE", Int64),
   ck("EXCL", Int64),
   ck("CHECKCONS", Int64),
   ck("OVERWRITE", Int64),
   ck("FL_NOCASE", Int64),
   ck("FL_NODIR", Int64),
   ck("FL_COMPRESSED", Int64),
   ck("FL_UNCHANGED", Int64),
   ck("CM_DEFAULT", Int64),
   ck("CM_STORE", Int64),
   ck("CM_SHRINK", Int64),
   ck("CM_REDUCE_1", Int64),
   ck("CM_REDUCE_2", Int64),
   ck("CM_REDUCE_3", Int64),
   ck("CM_REDUCE_4", Int64),
   ck("CM_IMPLODE", Int64),
   ck("CM_DEFLATE", Int64),
   ck("CM_DEFLATE64", Int64),
   ck("CM_PKWARE_IMPLODE", Int64),
   ck("ER_OK",   Int64),   
   ck("ER_MULTIDISK", Int64), 
   ck("ER_RENAME",  Int64),  
   ck("ER_CLOSE",  Int64),  
   ck("ER_SEEK",  Int64),  
   ck("ER_READ",  Int64),  
   ck("ER_WRITE",  Int64),  
   ck("ER_CRC",   Int64),  
   ck("ER_ZIPCLOSED", Int64), 
   ck("ER_NOENT",  Int64),  
   ck("ER_EXISTS",  Int64),  
   ck("ER_OPEN",  Int64),  
   ck("ER_TMPOPEN",  Int64), 
   ck("ER_ZLIB",  Int64),  
   ck("ER_MEMORY",  Int64),  
   ck("ER_CHANGED",  Int64), 
   ck("ER_COMPNOTSUPP", Int64),
   ck("ER_EOF",   Int64),  
   ck("ER_INVAL",  Int64),  
   ck("ER_NOZIP",  Int64),  
   ck("ER_INTERNAL", Int64), 
   ck("ER_INCONS",  Int64),  
   ck("ER_REMOVE",  Int64),  
   ck("ER_DELETED",   Int64), 
  ),
  "\n  private: ".
  "\n    struct zip *m_archive;",
  array(
    p(PublicProperty, 'status', Int32),
    p(PublicProperty, 'statusSys', Int32), 
    p(PublicProperty, 'numFiles', Int32), 
    p(PublicProperty, 'filename', String), 
    p(PublicProperty, 'comment', String)
  )
);

f('zip_close', null,
  array('zip' => Resource));

f('zip_entry_close', Boolean,
  array('zip_entry' => Resource));

f('zip_entry_compressedsize', Int64,
  array('zip_entry' => Resource));

f('zip_entry_compressionmethod', String,
  array('zip_entry' => Resource));

f('zip_entry_filesize', Int64,
  array('zip_entry' => Resource));

f('zip_entry_name', String,
  array('zip_entry' => Resource));

f('zip_entry_open', Boolean,
  array('zip' => Resource,
        'zip_entry' => Resource,
        'mode' => array(String, 'null_string')));

f('zip_entry_read', Variant,
  array('zip_entry' => Resource,
        'length' => array(Int64, '1024')));

f('zip_open', Variant,
  array('filename' => String));

f('zip_read', Variant,
  array('zip' => Resource));


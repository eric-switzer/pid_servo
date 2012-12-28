try: import simplejson as json
except ImportError: import json
import tempfile
import urllib2


def _decode_list(data):
    r"""convert native json unicode to str"""
    rv = []
    for item in data:
        if isinstance(item, unicode):
            item = item.encode('utf-8')
        elif isinstance(item, list):
            item = _decode_list(item)
        elif isinstance(item, dict):
            item = _decode_dict(item)
        rv.append(item)
    return rv


def _decode_dict(data):
    r"""convert native json unicode to str"""
    rv = {}
    for key, value in data.iteritems():
        if isinstance(key, unicode):
           key = key.encode('utf-8')
        if isinstance(value, unicode):
           value = value.encode('utf-8')
        elif isinstance(value, list):
           value = _decode_list(value)
        elif isinstance(value, dict):
           value = _decode_dict(value)
        rv[key] = value
    return rv


def load_json_over_http(url):
    req = urllib2.Request(url)
    opener = urllib2.build_opener()
    fp_url = opener.open(req)
    retjson = json.load(fp_url, object_hook=_decode_dict)

    return retjson


def load_json_over_http_file(url):
    r"""alternate implementation which writes a file"""
    req = urllib2.urlopen(url)
    temp_file = tempfile.NamedTemporaryFile()

    chunksize = 16 * 1024
    while True:
        chunk = req.read(chunksize)
        if not chunk: break
        temp_file.write(chunk)
    temp_file.flush()

    retjson = json.load(open(temp_file.name, "r"), object_hook=_decode_dict)
    temp_file.close()

    return retjson


def load_json(locator):
    if "http://" in locator:
        print "loading from website: ", locator
        database = load_json_over_http(locator)
    else:
        print "loading from file: ", locator
        fp = open(locator)
        database = json.load(fp, object_hook=_decode_dict)

    return database


if __name__ == "__main__":
    master_config = load_json("http://www.cita.utoronto.ca/~eswitzer/master.json")
    master_config = load_json("pid_servo.json")
    print master_config

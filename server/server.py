import sqlite3
import datetime
song_db = "__HOME__/songs.db"

def handle_post(request):
	conn = sqlite3.connect(song_db)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS song_table (song_name text,song text, timing timestamp);''')

	songname = request['values']['name']
	song = request['values']['song']

	c.execute('''INSERT into song_table VALUES (?,?,?);''', (songname,song,datetime.datetime.now()))
	conn.commit()
    conn.close()

	return "Song uploaded!"

def request_handler(request):
	if request['method'] == 'POST':
		return handle_post(request)
  	return request

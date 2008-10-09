import os
import cgi
import wsgiref.handlers

from google.appengine.api import users
from google.appengine.api import images
from google.appengine.ext import db
from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

class Greeting(db.Model):
  author = db.UserProperty()
  content = db.StringProperty(multiline=True)
  avatar = db.BlobProperty()
  date = db.DateTimeProperty(auto_now_add=True)

class MainPage(webapp.RequestHandler):
  def get(self):
    #greetings_query = Greeting.all().order('-date')    
    #greetings = greetings_query.fetch(10)
    greetings = db.GqlQuery("Select * from Greeting Order by date DESC limit 10")
    #greetings = db.GqlQuery("Select * from Greeting where author=:author Order by date DESC limit 10", author=users.get_current_user())
    if users.get_current_user():
      url = users.create_logout_url(self.request.uri)
      url_linktext = 'Logout'
    else:
      url = users.create_login_url(self.request.uri)
      url_linktext = 'Login'  
    caption = "Fool To-do List"
    template_values = {
      'greetings': greetings,
      'url': url,
      'url_linktext': url_linktext,
      'caption': caption,
      }
    path = os.path.join(os.path.dirname(__file__), 'index.html')
    self.response.out.write(template.render(path, template_values))

class Guestbook(webapp.RequestHandler):
  def post(self):
    greeting = Greeting()
    if users.get_current_user():
      greeting.author = users.get_current_user()
    greeting.content = self.request.get('content')
    avatar =  images.resize(self.request.get("img"), 32, 32)
    greeting.avatar = db.Blob(avatar)
    greeting.put()
    self.redirect('/')
    
class Image (webapp.RequestHandler):
  def get(self):
    greeting = db.get(self.request.get("img_id"))
    if greeting.avatar:
      self.response.headers['Content-Type'] = "image/jpeg"
      self.response.out.write(greeting.avatar)
    else:
      self.error(404)    

def main():
  application = webapp.WSGIApplication(
                                       [('/', MainPage),
                                        ('/img', Image),
                                        ('/sign', Guestbook)],
                                       debug=True)
  wsgiref.handlers.CGIHandler().run(application)

if __name__ == "__main__":
  main()
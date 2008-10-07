import wsgiref.handlers

from google.appengine.api import users
from google.appengine.ext import webapp

class MainPage(webapp.RequestHandler):
  def get(self):
    user = users.get_current_user()

    if user:
      self.response.headers['Content-Type'] = 'text/plain'
      self.response.out.write('Hello, ' + user.nickname() + ', and your e-mail is ' + user.email() + '.')
      admin = users.is_current_user_admin()
      if admin:
      	self.response.out.write('\nYou are administrator.')
      self.response.out.write('\n <a ref=\'' + users.create_logout_url(self.request.uri) + '\'>Logout</a>')
    else:
      self.redirect(users.create_login_url(self.request.uri))

def main():
  application = webapp.WSGIApplication(
                                       [('/', MainPage)],
                                       debug=True)
  wsgiref.handlers.CGIHandler().run(application)

if __name__ == "__main__":
  main()
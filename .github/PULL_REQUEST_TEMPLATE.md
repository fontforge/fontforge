Provide a general summary of your changes in the **Title** above.

### Important
Mark with [x] to select. Leave as [ ] to unselect.

### Motivation and Context
- [ ] Why is this change required? What problem does it solve?
- [ ] If it fixes an open issue, include the text `Closes #1` (where 1 would be the issue number) to your commit message.

### Types of changes
What types of changes does your code introduce? Check all the boxes that apply:
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to change)

### Description
- [ ] Describe your changes in detail.

### Final checklist:
Go over all the following points and check all the boxes that apply. 
If you're unsure about any of these, don't hesitate to ask. We're here to help! Various areas of the codebase have been worked on by different people in recent years, so if you are unfamiliar with the general area you're working in, please feel free to chat with people who have experience in that area. See the list [here](https://github.com/fontforge/fontforge/blob/master/CONTRIBUTING.md#people-to-ask).
- [ ] My code follows the code style of this project found [here](https://github.com/fontforge/fontforge/blob/master/CONTRIBUTING.md#coding-style).
- [ ] My change requires a change to the documentation.
- [ ] I have updated the documentation accordingly.
- [ ] I have read the [**CONTRIBUTING**](./CONTRIBUTING.md) guidelines.

Your pull request will be tested via Travis CI to automatically indicate that your changes do not prevent compilation. FontForge is a big program, so Travis can easily take over 20 minutes to confirm your changes are buildable. Please be patient. More details about using Travis can be found [here](https://github.com/fontforge/fontforge/blob/master/CONTRIBUTING.md#using-travis-ci).  
  
If it reports back that there are problems, you can log into the Travis system and check the log report for your pull request to see what the problem was. If no error is shown, just re-run the Travis test for your pull-request (that failed) to see a fresh report since the last report may be for someone else that did a later pull request, or for mainline code. If you add new code to fix your issue/problem, then take note that you need to check the next pull request in the Travis system. Travis issue numbers are different from GitHub issue numbers.
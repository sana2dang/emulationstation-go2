#include "views/gamelist/BasicGameListView.h"

#include "utils/FileSystemUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"

using namespace std;
 
BasicGameListView::BasicGameListView(Window* window, FileData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	mList.setDefaultZIndex(20);
	addChild(&mList);

	populateList(root->getChildrenListToDisplay());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);

	sortChildren();
}

void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if(change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}


void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
	mList.clear();
	std::string systemName = mRoot->getSystem()->getFullName();
	mHeaderText.setText(mRoot->getSystem()->getFullName());

	if (files.size() > 0)
	{
		std::string systemName = mRoot->getSystem()->getName();
		bool favoritesFirst = Settings::getInstance()->getBool("FavoritesFirst");
		bool showFavoriteIcon = (systemName != "즐겨찾기");
		bool folderFirst = true;
		if (!showFavoriteIcon)
			favoritesFirst = false;

		// 0 : 일반
		// 1 : 폴더
		// 2 : 즐겨찾기
		// 3 : 한글
		// 4 : 성인
		std::string korea[] = {"한글", "한국", "korean", "kor"};
				
		// 폴더를 최상단에 표시
		if (folderFirst)
		{
			for (auto it = files.cbegin(); it != files.cend(); it++)
			{
				if ((*it)->getType() == FOLDER)
					mList.add("#" + (*it)->getName(), *it, 1);
				else
					continue;
			}
		}
		
		// 즐겨찾기를 최상단에 표시
		if (favoritesFirst)
		{
			for (auto it = files.cbegin(); it != files.cend(); it++)
			{
				std::string filename = Utils::FileSystem::getFileName((*it)->getName());
		
				if (!(*it)->getFavorite())
					continue;
				
				if (showFavoriteIcon)
				{
					for(int i=0; i<4; i++)
					{
						if( filename.find(korea[i]) != string::npos )
						{
							mList.add("★" + (*it)->getName(), *it, 3);
							break;
						}
						if( i == 3)
							mList.add("★" + (*it)->getName(), *it, 2);
					}

					
				}
				else if ((*it)->getType() == FOLDER)
					continue;
				else
					mList.add((*it)->getName(), *it, 0);				
			}
		}

		// 일반 리스트 표시
		for(auto it = files.cbegin(); it != files.cend(); it++)
		{
			std::string filename = Utils::FileSystem::getFileName((*it)->getName());
			if ((*it)->getFavorite())
			{
				if (favoritesFirst)
					continue;
				
				if (showFavoriteIcon)
				{
					for(int i=0; i<4; i++)
					{
						if( filename.find(korea[i]) != string::npos )
						{
							mList.add("★" + (*it)->getName(), *it, 3);
							break;
						}
						if( i == 3)
							mList.add("★" + (*it)->getName(), *it, 2);
					}
					continue;
				}
			}

			if ((*it)->getType() == FOLDER)
				continue; //mList.add(" #" + (*it)->getName(), *it, true);
			else
			{
				for(int i=0; i<4; i++)
				{
					if( filename.find(korea[i]) != string::npos )
					{
						mList.add((*it)->getName(), *it, 3);
						break;
					}
					if( i == 3)
						mList.add((*it)->getName(), *it, 0);
				}
			}
		}
	}
	else
	{
		addPlaceholder();
	}
}

FileData* BasicGameListView::getCursor()
{
	return mList.getSelected();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	if(!mList.setCursor(cursor) && (!cursor->isPlaceHolder()))
	{
		populateList(cursor->getParent()->getChildrenListToDisplay());
		mList.setCursor(cursor);

		// update our cursor stack in case our cursor just got set to some folder we weren't in before
		if(mCursorStack.empty() || mCursorStack.top() != cursor->getParent())
		{
			std::stack<FileData*> tmp;
			FileData* ptr = cursor->getParent();
			while(ptr && ptr != mRoot)
			{
				tmp.push(ptr);
				ptr = ptr->getParent();
			}

			// flip the stack and put it in mCursorStack
			mCursorStack = std::stack<FileData*>();
			while(!tmp.empty())
			{
				mCursorStack.push(tmp.top());
				tmp.pop();
			}
		}
	}
}

void BasicGameListView::addPlaceholder()
{
	// empty list - add a placeholder
	

	FileData* placeholder = new FileData(PLACEHOLDER, "<리스트 없음>", this->mRoot->getSystem()->getSystemEnvData(), this->mRoot->getSystem());	
	mList.add(placeholder->getName(), placeholder, (placeholder->getType() == PLACEHOLDER));

}

std::string BasicGameListView::getQuickSystemSelectRightButton()
{
	return "right";
}

std::string BasicGameListView::getQuickSystemSelectLeftButton()
{
	return "left";
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void BasicGameListView::remove(FileData *game, bool deleteFile)
{
	if (deleteFile)
		Utils::FileSystem::removeFile(game->getPath());  // actually delete the file on the filesystem
	FileData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
		unsigned int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
		if (gameIter != siblings.cend())
		{
			if ((gamePos + 1) < siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if (gamePos > 1) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	mList.remove(game);
	if(mList.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
}

std::vector<HelpPrompt> BasicGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("left/right", "시스템"));
	prompts.push_back(HelpPrompt("up/down", "이동"));
	prompts.push_back(HelpPrompt("a", "실행"));
	prompts.push_back(HelpPrompt("b", "이전"));

	if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("select", "옵션"));
	if(mRoot->getSystem()->isGameSystem())
		prompts.push_back(HelpPrompt("x", "무작위"));
	if(mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid())
	{
		std::string prompt = CollectionSystemManager::get()->getEditingCollection();
		prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}
